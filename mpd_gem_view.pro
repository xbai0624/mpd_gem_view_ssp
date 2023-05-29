TEMPLATE = subdirs

#SUBDIRS = decoder gem epics gui fadc tracking_dev
SUBDIRS = decoder/decoder.pro gem/gem.pro epics/epics.pro gui/gui.pro fadc/fadc.pro \
          tracking_dev/tracking_dev.pro tracking_dev/tracking_dev_app.pro \
          replay/replay.pro alignment/alignment.pro
