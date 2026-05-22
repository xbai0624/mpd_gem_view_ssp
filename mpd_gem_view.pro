TEMPLATE = subdirs

SUBDIRS = decoder gem epics gui tracking_dev tracking_dev_app replay alignment

decoder.file             = decoder/decoder.pro

epics.file               = epics/epics.pro

gem.file                 = gem/gem.pro
gem.depends              = decoder

gui.file                 = gui/gui.pro
gui.depends              = decoder gem epics

# Optional online (ET) monitoring. Off by default; enable with:
#     qmake CONFIG+=et
# When enabled, build the ET client wrapper lib (under gui/online_monitor,
# never touching third_party) before gui, and have gui depend on it.
et {
    SUBDIRS               += online_monitor
    online_monitor.file    = gui/online_monitor/online_monitor.pro
    online_monitor.depends = decoder gem
    gui.depends           += online_monitor
}

tracking_dev.file        = tracking_dev/tracking_dev.pro
tracking_dev.depends     = decoder gem

tracking_dev_app.file    = tracking_dev/tracking_dev_app.pro
tracking_dev_app.depends = decoder gem tracking_dev

replay.file              = replay/replay.pro
replay.depends           = decoder gem tracking_dev

alignment.file           = alignment/alignment.pro
alignment.depends        = decoder gem tracking_dev
