project('pax_tar', 'cpp',
  version : '1.0.0',
  license : 'APACHE')

files = ['src/pax_tar.cpp']
all_args_lnk = ['-lstdc++fs','-lfmt']
paxLib = static_library('pax_tar', files, link_args : all_args_lnk, install : true)

install_headers('src/pax_tar.h', subdir : 'pax_tar') # -> include/pax_tar/pax_tar.h

