/**
  * @file    at_port.c
  * @brief   AT transport abstraction with a default mock backend
  */

#include "at_port.h"
#include "at_kernel.h"
#include "main.h"
#include <string.h>

#define AT_PORT_USE_MOCK        1
#define AT_MOCK_RESPONSE_DELAY  50U

extern uint32_t __start_at_cmd_table[];
extern uint32_t __stop_at_cmd_table[];

static char g_pending_response[128];
static uint32_t g_pending_tick = 0U;
static uint8_t g_has_pending_response = 0U;

static const AT_CMD_T *at_find_registered_command(const char *command);
static const char *at_get_expected_response(const AT_CMD_T *cmd);
static void at_prepare_mock_response(const AT_CMD_T *cmd);

void at_port_send(const uint8_t *data, uint16_t len)
{
    char command[32];
    uint16_t copy_len;

    if (data == NULL || len == 0U) {
        return;
    }

#if AT_PORT_USE_MOCK
    const AT_CMD_T *registered_cmd;

    copy_len = len;
    if (copy_len >= sizeof(command)) {
        copy_len = sizeof(command) - 1U;
    }

    memcpy(command, data, copy_len);
    command[copy_len] = '\0';

    while (copy_len > 0U &&
           (command[copy_len - 1U] == '\r' || command[copy_len - 1U] == '\n')) {
        command[--copy_len] = '\0';
    }

    if (copy_len == 0U) {
        return;
    }

    registered_cmd = at_find_registered_command(command);
    if (registered_cmd != NULL) {
        at_prepare_mock_response(registered_cmd);
        g_pending_tick = HAL_GetTick() + AT_MOCK_RESPONSE_DELAY;
        g_has_pending_response = 1U;
    }
#else
    HAL_UART_Transmit(&huart1, (uint8_t *)data, len, 1000);
#endif
}

uint32_t at_port_get_tick(void)
{
    return HAL_GetTick();
}

void at_port_poll(void)
{
#if AT_PORT_USE_MOCK
    if (g_has_pending_response == 0U) {
        return;
    }

    if ((int32_t)(HAL_GetTick() - g_pending_tick) >= 0) {
        at_receive_data((const uint8_t *)g_pending_response, (uint16_t)strlen(g_pending_response));
        g_has_pending_response = 0U;
        g_pending_response[0] = '\0';
    }
#endif
}

static const AT_CMD_T *at_find_registered_command(const char *command)
{
    const AT_CMD_T *cmd_table = (const AT_CMD_T *)__start_at_cmd_table;
    const AT_CMD_T *cmd_table_end = (const AT_CMD_T *)__stop_at_cmd_table;

    if (command == NULL) {
        return NULL;
    }

    while (cmd_table < cmd_table_end) {
        if (cmd_table->cmd_string != NULL && strcmp(command, cmd_table->cmd_string) == 0) {
            return cmd_table;
        }
        cmd_table++;
    }

    return NULL;
}

static const char *at_get_expected_response(const AT_CMD_T *cmd)
{
    if (cmd == NULL || cmd->expected_response == NULL || cmd->expected_response[0] == '\0') {
        return "OK";
    }

    return cmd->expected_response;
}

static void at_prepare_mock_response(const AT_CMD_T *cmd)
{
    const char *expected_response = at_get_expected_response(cmd);
    size_t expected_len;

    g_pending_response[0] = '\0';

    if (cmd == NULL || cmd->callback == NULL || strcmp(expected_response, "OK") == 0 || strcmp(expected_response, "*") == 0) {
        strncpy(g_pending_response, "OK\r\n", sizeof(g_pending_response) - 1U);
        g_pending_response[sizeof(g_pending_response) - 1U] = '\0';
        return;
    }

    expected_len = strlen(expected_response);
    if (expected_len >= (sizeof(g_pending_response) - 1U)) {
        expected_len = sizeof(g_pending_response) - 1U;
    }

    memcpy(g_pending_response, expected_response, expected_len);
    g_pending_response[expected_len] = '\0';

    if ((sizeof(g_pending_response) - expected_len) > 6U) {
        strncat(g_pending_response, "\r\nOK\r\n", sizeof(g_pending_response) - strlen(g_pending_response) - 1U);
    }
}
