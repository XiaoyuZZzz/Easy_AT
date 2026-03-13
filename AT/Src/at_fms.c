/**
  * @file    at_fms.c
  * @brief   AT command engine core implementation
  */

#include "at_kernel.h"
#include "at_port.h"
#include <stdio.h>
#include <string.h>

#define AT_MAX_RESPONSE_LEN     128U
#define AT_MAX_CMD_COUNT        64U
#define AT_INVALID_INDEX        0xFFFFU

typedef struct {
    char line[AT_MAX_RESPONSE_LEN];
    uint16_t line_len;
    bool line_ready;
} AT_RX_BUFFER_T;

extern uint32_t __start_at_cmd_table[];
extern uint32_t __stop_at_cmd_table[];

static AT_CMD_RUNTIME_T g_runtime_states[AT_MAX_CMD_COUNT];
static AT_MANAGER_T g_at_manager = {0};
static AT_RX_BUFFER_T g_rx_buffer = {0};
static void (*g_timeout_callback)(const char *cmd_name) = NULL;
static const AT_CMD_T *g_current_cmd = NULL;
static uint16_t g_active_index = AT_INVALID_INDEX;

static void at_parse_response(void);
static bool at_check_response(const char *response, const char *expected);
static void at_execute_next_command(void);
static void at_send_command(const AT_CMD_T *cmd);
static void at_check_periodic_commands(void);
static void at_execute_command_by_index(uint16_t index);
static const char *at_get_expected_response(const AT_CMD_T *cmd);

void at_init(void)
{
    g_at_manager.cmd_table = (const AT_CMD_T *)__start_at_cmd_table;
    g_at_manager.cmd_table_end = (const AT_CMD_T *)__stop_at_cmd_table;
    g_at_manager.cmd_count = (uint16_t)(g_at_manager.cmd_table_end - g_at_manager.cmd_table);
    if (g_at_manager.cmd_count > AT_MAX_CMD_COUNT) {
        g_at_manager.cmd_count = AT_MAX_CMD_COUNT;
    }

    g_at_manager.current_index = 0U;
    g_at_manager.is_busy = false;
    g_at_manager.current_phase = AT_PHASE_INIT;
    g_at_manager.init_completed = false;
    g_at_manager.runtime_states = g_runtime_states;

    for (uint16_t i = 0; i < g_at_manager.cmd_count; i++) {
        g_runtime_states[i].state = AT_CMD_STATE_IDLE;
        g_runtime_states[i].start_tick = 0U;
        g_runtime_states[i].last_exec_tick = 0U;
        g_runtime_states[i].executed = false;
    }

    memset(&g_rx_buffer, 0, sizeof(g_rx_buffer));
    g_current_cmd = NULL;
    g_active_index = AT_INVALID_INDEX;

    printf("[AT] Initialized. Command table: %u commands\r\n", at_get_cmd_count());
}

void at_start(void)
{
    if (g_at_manager.is_busy) {
        return;
    }

    for (uint16_t i = 0; i < g_at_manager.cmd_count; i++) {
        g_runtime_states[i].state = AT_CMD_STATE_IDLE;
        g_runtime_states[i].start_tick = 0U;
        g_runtime_states[i].last_exec_tick = 0U;
        g_runtime_states[i].executed = false;
    }

    memset(&g_rx_buffer, 0, sizeof(g_rx_buffer));
    g_at_manager.current_index = 0U;
    g_at_manager.is_busy = true;
    g_at_manager.current_phase = AT_PHASE_INIT;
    g_at_manager.init_completed = false;
    g_current_cmd = NULL;
    g_active_index = AT_INVALID_INDEX;

    at_execute_next_command();
}

void at_stop(void)
{
    g_at_manager.is_busy = false;
    g_current_cmd = NULL;
    g_active_index = AT_INVALID_INDEX;
}

void at_process(void)
{
    at_port_poll();

    if (g_at_manager.is_busy && g_current_cmd != NULL && g_active_index < g_at_manager.cmd_count) {
        AT_CMD_RUNTIME_T *runtime = &g_runtime_states[g_active_index];
        uint32_t current_tick = at_port_get_tick();

        if ((current_tick - runtime->start_tick) >= g_current_cmd->timeout_ms) {
            printf("[AT] Timeout: %s\r\n", g_current_cmd->name);

            if (g_timeout_callback != NULL) {
                g_timeout_callback(g_current_cmd->name);
            }

            runtime->state = AT_CMD_STATE_TIMEOUT;
            runtime->executed = true;
            runtime->last_exec_tick = current_tick;
            at_execute_next_command();
            return;
        }

        at_parse_response();
    }

    if (g_at_manager.init_completed && !g_at_manager.is_busy) {
        at_check_periodic_commands();
    }
}

void at_receive_data(const uint8_t *data, uint16_t len)
{
    if (data == NULL) {
        return;
    }

    for (uint16_t i = 0; i < len; i++) {
        char ch = (char)data[i];

        if (ch == '\r') {
            continue;
        }

        if (ch == '\n') {
            if (g_rx_buffer.line_len > 0U) {
                g_rx_buffer.line[g_rx_buffer.line_len] = '\0';
                g_rx_buffer.line_ready = true;
            }
            continue;
        }

        if (g_rx_buffer.line_ready) {
            g_rx_buffer.line_ready = false;
            g_rx_buffer.line_len = 0U;
            g_rx_buffer.line[0] = '\0';
        }

        if (g_rx_buffer.line_len < (AT_MAX_RESPONSE_LEN - 1U)) {
            g_rx_buffer.line[g_rx_buffer.line_len++] = ch;
        }
    }
}

AT_CMD_STATE_T at_get_cmd_state(const char *cmd_name)
{
    const AT_CMD_T *cmd = g_at_manager.cmd_table;
    uint16_t index = 0U;

    while (cmd < g_at_manager.cmd_table_end && index < g_at_manager.cmd_count) {
        if (strcmp(cmd->name, cmd_name) == 0) {
            return g_runtime_states[index].state;
        }
        cmd++;
        index++;
    }

    return AT_CMD_STATE_ERROR;
}

uint16_t at_get_current_index(void)
{
    return g_at_manager.current_index;
}

uint16_t at_get_cmd_count(void)
{
    return g_at_manager.cmd_count;
}

void at_set_timeout_callback(void (*cb)(const char *cmd_name))
{
    g_timeout_callback = cb;
}

AT_PHASE_T at_get_current_phase(void)
{
    return g_at_manager.current_phase;
}

void at_trigger_periodic_cmd(const char *cmd_name)
{
    const AT_CMD_T *cmd = g_at_manager.cmd_table;
    uint16_t index = 0U;

    while (cmd < g_at_manager.cmd_table_end && index < g_at_manager.cmd_count) {
        if (strcmp(cmd->name, cmd_name) == 0) {
            if (cmd->type == AT_CMD_TYPE_PERIODIC) {
                g_runtime_states[index].executed = false;
                g_runtime_states[index].last_exec_tick = 0U;
                printf("[AT] Trigger periodic cmd: %s\r\n", cmd_name);
            }
            return;
        }
        cmd++;
        index++;
    }
}

static void at_check_periodic_commands(void)
{
    uint32_t current_tick = at_port_get_tick();

    for (uint16_t i = 0; i < g_at_manager.cmd_count; i++) {
        const AT_CMD_T *cmd = &g_at_manager.cmd_table[i];
        AT_CMD_RUNTIME_T *runtime = &g_runtime_states[i];

        if (cmd->type != AT_CMD_TYPE_PERIODIC) {
            continue;
        }

        if (!runtime->executed ||
            ((current_tick - runtime->last_exec_tick) >= cmd->interval_ms)) {
            at_execute_command_by_index(i);
            return;
        }
    }
}

static void at_execute_command_by_index(uint16_t index)
{
    if (index >= g_at_manager.cmd_count) {
        return;
    }

    g_active_index = index;
    g_current_cmd = &g_at_manager.cmd_table[index];
    g_at_manager.current_index = index + 1U;
    g_at_manager.is_busy = true;

    printf("[AT] Executing: %s (%s)\r\n", g_current_cmd->name, g_current_cmd->cmd_string);
    at_send_command(g_current_cmd);
}

static void at_parse_response(void)
{
    AT_CMD_RUNTIME_T *runtime;
    const char *expected_response;

    if (!g_rx_buffer.line_ready || g_current_cmd == NULL || g_active_index >= g_at_manager.cmd_count) {
        return;
    }

    runtime = &g_runtime_states[g_active_index];
    expected_response = at_get_expected_response(g_current_cmd);

    if (at_check_response(g_rx_buffer.line, expected_response)) {
        printf("[AT] Matched: %s -> %s\r\n", g_current_cmd->name, expected_response);

        strncpy(g_at_manager.response_buffer, g_rx_buffer.line, sizeof(g_at_manager.response_buffer) - 1U);
        g_at_manager.response_buffer[sizeof(g_at_manager.response_buffer) - 1U] = '\0';

        if (g_current_cmd->callback != NULL) {
            g_current_cmd->callback(g_at_manager.response_buffer, g_current_cmd->user_data);
        }

        runtime->state = AT_CMD_STATE_COMPLETED;
        runtime->executed = true;
        runtime->last_exec_tick = at_port_get_tick();
        g_rx_buffer.line_ready = false;
        g_rx_buffer.line_len = 0U;
        g_rx_buffer.line[0] = '\0';
        at_execute_next_command();
        return;
    }

    g_rx_buffer.line_ready = false;
    g_rx_buffer.line_len = 0U;
    g_rx_buffer.line[0] = '\0';
}

static bool at_check_response(const char *response, const char *expected)
{
    if (response == NULL || expected == NULL) {
        return false;
    }

    if (strcmp(expected, "*") == 0) {
        return strlen(response) > 0U;
    }

    return strstr(response, expected) != NULL;
}

static const char *at_get_expected_response(const AT_CMD_T *cmd)
{
    if (cmd == NULL || cmd->expected_response == NULL || cmd->expected_response[0] == '\0') {
        return "OK";
    }

    return cmd->expected_response;
}

static void at_execute_next_command(void)
{
    while (g_at_manager.current_index < g_at_manager.cmd_count) {
        const AT_CMD_T *cmd = &g_at_manager.cmd_table[g_at_manager.current_index];

        if (g_at_manager.current_phase == AT_PHASE_INIT) {
            if (cmd->type == AT_CMD_TYPE_ONCE) {
                g_active_index = g_at_manager.current_index;
                g_current_cmd = cmd;
                g_at_manager.current_index++;
                printf("[AT] Executing: %s (%s)\r\n", g_current_cmd->name, g_current_cmd->cmd_string);
                at_send_command(g_current_cmd);
                return;
            }

            g_at_manager.current_index++;
            continue;
        }

        if (g_at_manager.current_phase == AT_PHASE_LOOP) {
            g_at_manager.is_busy = false;
            g_current_cmd = NULL;
            g_active_index = AT_INVALID_INDEX;
            return;
        }
    }

    if (g_at_manager.current_phase == AT_PHASE_INIT) {
        printf("[AT] Initialization phase completed\r\n");
        g_at_manager.init_completed = true;
        g_at_manager.current_phase = AT_PHASE_LOOP;
        g_at_manager.current_index = 0U;
    } else {
        printf("[AT] All commands completed\r\n");
    }

    g_at_manager.is_busy = false;
    g_current_cmd = NULL;
    g_active_index = AT_INVALID_INDEX;
}

static void at_send_command(const AT_CMD_T *cmd)
{
    AT_CMD_RUNTIME_T *runtime;

    if (cmd == NULL || g_active_index >= g_at_manager.cmd_count) {
        return;
    }

    runtime = &g_runtime_states[g_active_index];
    runtime->state = AT_CMD_STATE_WAITING;
    runtime->start_tick = at_port_get_tick();

    at_port_send((const uint8_t *)cmd->cmd_string, (uint16_t)strlen(cmd->cmd_string));
    at_port_send((const uint8_t *)"\r\n", 2U);
}
