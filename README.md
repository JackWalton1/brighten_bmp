# brighten_bmp
brightens bmp images using parallel processing if specified in cmd line

try: 
gcc fork.c -o fork
./fork wolf.bmp 1 0 brightWolf.bmp (./program [bmpFile] [Brightness(0-1)] [Parallel(0/1)] [bmpOutFile])
