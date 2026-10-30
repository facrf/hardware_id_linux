# Bash completion for hardware_id
_hardware_id() {
    local cur prev words cword
    _init_completion || return

    local options="
        -j --json
        -s --only-id
        --uuid
        -f --fast
        -w --watch
        --include-identifiers
        --redact
        --ascii
        --no-color
        -h --help
        -v --version
    "

    case "$prev" in
        -w|--watch)
            COMPREPLY=( $(compgen -W "1 2 3 5 10" -- "$cur") )
            return 0
            ;;
    esac

    if [[ "$cur" == -* ]]; then
        COMPREPLY=( $(compgen -W "$options" -- "$cur") )
        return 0
    fi
}

complete -F _hardware_id hardware_id
