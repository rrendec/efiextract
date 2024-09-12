efiextract
==========

Bootable Linux kernel images may be packaged as EFI zboot images, which are
self-decompressing executables when loaded via EFI. `efiextract` is a small
tool that can extract the payload from such images and write the original Linux
kernel image to a separate file.

This program is heavily inspired by https://github.com/eballetbo/unzboot but
has no other build requirements than a C compiler and `make`. Also, it does not
decompress the payload.
