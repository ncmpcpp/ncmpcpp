# Installation:
# - If you have system bash completion, place this in /etc/bash_completion.d or
#   source it from $HOME/.bash_completion
# - If you don't have system bash completion, source this from your .bashrc

 _ncmpcpp ()
{
    local cur prev cmds opts

    cur="$2"
    prev="$3"
    cmds=`ncmpcpp --help 2>&1 | awk '/^  [a-z]+ /{print $1}'`;
    opts="`ncmpcpp --help 2>&1 | awk '/^  -.+,/{print $1 "\n" $2}' | sed -e 's/,//;$a--now-playing'`"

    case "$prev" in
        ncmpcpp)
        COMPREPLY=($(compgen -W '$opts $cmds' -- "$cur"))
        ;;
        --port|-p|--host|-h)
        COMPREPLY=()
        ;;
        --help|-?|--version|-v|--now-playing)
        return
        ;;
        next|pause|play|prev|stop|toggle|volume)
        opts="`echo $opts | sed -e 's/--port//;s/--host//;s/-p//;s/-h//'`"
        COMPREPLY=($(compgen -W '$opts' -- "$cur"))
        ;;
        *)
        COMPREPLY=($(compgen -W '$opts $cmds' -- "$cur"))
        ;;
    esac
}
complete -F _ncmpcpp ncmpcpp
