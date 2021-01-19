
 Preloader-M1
 ~~~~~~~~~~~~~

To build:

 * make sure you have the appropriate toolchain (see Makefile),
 * put links to Image and apple-m1-j274.dtb from kernel build in cwd,
 * run make.

This results in a file called test.macho.

To use macho: start your mac in 1TR (hold power button during boot, select Options)
and run the following commands:

  bputil -n -k -c -a -s
  csrutil disable
  csrutil authenticated-root disable

The above commands need to be run only once.

To install an image:

  kmutil configure-boot -c test.macho -v /Volumes/Macintosh HD/

To revert back to original Mac OS boot:

  bputil -n pol change

