cmds = []
cmds << 'gcc -Wall -lspotify example.c -o example'
cmds << './example'

system cmds.join(' && ')