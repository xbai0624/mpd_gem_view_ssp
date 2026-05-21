TEMPLATE = subdirs

SUBDIRS = decoder gem epics gui tracking_dev tracking_dev_app replay alignment

decoder.file             = decoder/decoder.pro

epics.file               = epics/epics.pro

gem.file                 = gem/gem.pro
gem.depends              = decoder

gui.file                 = gui/gui.pro
gui.depends              = decoder gem epics

tracking_dev.file        = tracking_dev/tracking_dev.pro
tracking_dev.depends     = decoder gem

tracking_dev_app.file    = tracking_dev/tracking_dev_app.pro
tracking_dev_app.depends = decoder gem tracking_dev

replay.file              = replay/replay.pro
replay.depends           = decoder gem tracking_dev

alignment.file           = alignment/alignment.pro
alignment.depends        = decoder gem tracking_dev
