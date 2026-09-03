subdir('dbus')
subdir('dynamic')
subdir('max')
if libatasmart_dep.found()
  subdir('udisks')
endif

if get_option('enable-fake')
  subdir('fake')
endif

if libsensors_dep.found()
  subdir('libsensors')
endif

if get_option('enable-nvidia') and x11_dep.found() and libxnvctrl_dep.found()
  subdir('nvidia')
endif

if udisks2_dep.found()
  subdir('udisks2')
endif
