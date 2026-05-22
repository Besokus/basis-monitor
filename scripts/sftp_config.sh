#!/bin/sh

load_sftp_config() {
    script_dir="$1"
    project_dir="$2"

    config_file="${SFTP_CONFIG_FILE:-$project_dir/config/sftp.conf}"
    if [ -f "$config_file" ]; then
        # shellcheck disable=SC1090
        . "$config_file"
    fi

    SFTP_HOST="${SFTP_HOST:-}"
    SFTP_USER="${SFTP_USER:-}"
    SFTP_PORT="${SFTP_PORT:-22}"
    SFTP_IDENTITY_FILE="${SFTP_IDENTITY_FILE:-}"

    if [ -n "$SFTP_IDENTITY_FILE" ] && [ ! -f "$SFTP_IDENTITY_FILE" ]; then
        echo "Configured SFTP_IDENTITY_FILE does not exist: $SFTP_IDENTITY_FILE" >&2
        exit 1
    fi
}

build_sftp_target() {
    explicit_target="$1"

    if [ -n "$explicit_target" ]; then
        echo "$explicit_target"
        return
    fi

    if [ -z "${SFTP_USER:-}" ] || [ -z "${SFTP_HOST:-}" ]; then
        echo "Missing SFTP target. Pass <user@host> or set SFTP_USER and SFTP_HOST in config/sftp.conf." >&2
        exit 1
    fi

    echo "${SFTP_USER}@${SFTP_HOST}"
}
