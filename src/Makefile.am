HEADER_PATH = ../include
include ${top_srcdir}/moc4.mk

#bin_PROGRAMS = jserver \
#	       jclient \
#               fishgrid \
#               fishgridstepper \
#               fishgridrecorder
bin_PROGRAMS = fishgrid

if FISHGRID_COND_COMEDI
bin_PROGRAMS += fishgridcalibcomedi
endif



jserver_CPPFLAGS = \
    -I$(srcdir)/../include \
    $(QT4_CPPFLAGS) \
    $(GSL_CPPFLAGS) \
    $(RELACS_LIBS_CPPFLAGS)

jserver_LDFLAGS = \
    $(QT4_LDFLAGS) \
    $(GSL_LDFLAGS) \
    $(RELACS_LIBS_LDFLAGS)

jserver_LDADD = \
    $(QT4_LIBS) \
    $(GSL_LIBS) \
    $(RELACS_LIBS_LIBS)

JSERVER_MOCFILES =

$(jserver_OBJECTS) : ${JSERVER_MOCFILES}

jserver_SOURCES = \
    jserver.cc



jclient_CPPFLAGS = \
    -I$(srcdir)/../include \
    $(QT4_CPPFLAGS) \
    $(GSL_CPPFLAGS) \
    $(RELACS_LIBS_CPPFLAGS)

jclient_LDFLAGS = \
    $(QT4_LDFLAGS) \
    $(GSL_LDFLAGS) \
    $(RELACS_LIBS_LDFLAGS)

jclient_LDADD = \
    $(QT4_LIBS) \
    $(GSL_LIBS) \
    $(RELACS_LIBS_LIBS)

JCLIENT_MOCFILES =

$(jclient_OBJECTS) : ${JCLIENT_MOCFILES}

jclient_SOURCES = \
    jclient.cc



fishgrid_CPPFLAGS = \
    -I$(srcdir)/../include \
    $(QT4_CPPFLAGS) \
    $(GSL_CPPFLAGS) \
    $(RELACS_LIBS_CPPFLAGS)

fishgrid_LDFLAGS = \
    $(QT4_LDFLAGS) \
    $(GSL_LDFLAGS) \
    $(RELACS_LIBS_LDFLAGS)

fishgrid_LDADD = \
    $(QT4_LIBS) \
    $(GSL_LIBS) \
    $(RELACS_LIBS_LIBS)

if FISHGRID_COND_COMEDI
fishgrid_CPPFLAGS += $(COMEDI_CPPFLAGS) 
fishgrid_LDFLAGS += $(COMEDI_LDFLAGS)
fishgrid_LDADD += $(COMEDI_LIBS)
endif

if FISHGRID_COND_NIDAQMX
fishgrid_CPPFLAGS += $(FISHGRID_NIDAQMXBASE_CPPFLAGS) 
fishgrid_LDFLAGS += $(FISHGRID_NIDAQMXBASE_LDFLAGS)
fishgrid_LDADD += $(FISHGRID_NIDAQMXBASE_LIBS)
endif

if RELACS_COND_PORTAUDIO
fishgrid_CPPFLAGS += $(PORTAUDIO_CPPFLAGS) 
fishgrid_LDFLAGS += $(PORTAUDIO_LDFLAGS)
fishgrid_LDADD += $(PORTAUDIO_LIBS)
endif


FISHGRID_MOCFILES = \
    moc_basewidget.cc \
    moc_fishgridwidget.cc \
    moc_browsedatawidget.cc \
    moc_preprocessor.cc \
    moc_demean.cc \
    moc_commonnoiseremoval.cc \
    moc_analyzer.cc \
    moc_idle.cc \
    moc_traces.cc \
    moc_spectra.cc \
    moc_rmsplot.cc \
    moc_rmspixel.cc \
    moc_janalyzer.cc 

$(fishgrid_OBJECTS) : ${FISHGRID_MOCFILES}

fishgrid_SOURCES = \
    fishgrid.cc \
    configdata.cc ../include/configdata.h \
    basewidget.cc ../include/basewidget.h \
    fishgridwidget.cc ../include/fishgridwidget.h \
    browsedatawidget.cc ../include/browsedatawidget.h \
    datathread.cc ../include/datathread.h \
    simulationthread.cc ../include/simulationthread.h \
    preprocessor.cc ../include/preprocessor.h \
    demean.cc ../include/demean.h \
    commonnoiseremoval.cc ../include/commonnoiseremoval.h \
    analyzer.cc ../include/analyzer.h \
    idle.cc ../include/idle.h \
    traces.cc ../include/traces.h \
    spectra.cc ../include/spectra.h \
    rmsplot.cc ../include/rmsplot.h \
    rmspixel.cc ../include/rmspixel.h \
    janalyzer.cc ../include/janalyzer.h \
    recording.cc ../include/recording.h \
    ../include/cyclicbuffer.h
if FISHGRID_COND_COMEDI
fishgrid_SOURCES += \
    comedithread.cc ../include/comedithread.h
endif
if FISHGRID_COND_NIDAQMX
fishgrid_SOURCES += \
    nidaqmxthread.cc ../include/nidaqmxthread.h
endif
if RELACS_COND_PORTAUDIO
fishgrid_SOURCES += \
    portaudiomonitor.cc ../include/portaudiomonitor.h
endif


fishgridstepper_CPPFLAGS = \
    -I$(srcdir)/../include \
    $(QT4_CPPFLAGS) \
    $(GSL_CPPFLAGS) \
    $(RELACS_LIBS_CPPFLAGS)

fishgridstepper_LDFLAGS = \
    $(QT4_LDFLAGS) \
    $(GSL_LDFLAGS) \
    $(RELACS_LIBS_LDFLAGS)

fishgridstepper_LDADD = \
    $(QT4_LIBS) \
    $(GSL_LIBS) \
    $(RELACS_LIBS_LIBS)

if FISHGRID_COND_COMEDI
fishgridstepper_CPPFLAGS += $(COMEDI_CPPFLAGS) 
fishgridstepper_LDFLAGS += $(COMEDI_LDFLAGS)
fishgridstepper_LDADD += $(COMEDI_LIBS)
endif

if FISHGRID_COND_NIDAQMX
fishgridstepper_CPPFLAGS += $(FISHGRID_NIDAQMXBASE_CPPFLAGS) 
fishgridstepper_LDFLAGS += $(FISHGRID_NIDAQMXBASE_LDFLAGS)
fishgridstepper_LDADD += $(FISHGRID_NIDAQMXBASE_LIBS)
endif

FISHGRIDSTEPPER_MOCFILES = \
    moc_stepper.cc

$(fishgridstepper_OBJECTS) : ${FISHGRIDSTEPPER_MOCFILES}

fishgridstepper_SOURCES = \
    fishgridstepper.cc \
    configdata.cc ../include/configdata.h \
    stepper.cc ../include/stepper.h \
    datathread.cc ../include/datathread.h \
    simulationthread.cc ../include/simulationthread.h \
    recording.cc ../include/recording.h \
    ../include/cyclicbuffer.h
if FISHGRID_COND_COMEDI
fishgridstepper_SOURCES += \
    comedithread.cc ../include/comedithread.h
endif
if FISHGRID_COND_NIDAQMX
fishgridstepper_SOURCES += \
    nidaqmxthread.cc ../include/nidaqmxthread.h
endif



fishgridrecorder_CPPFLAGS = \
    -I$(srcdir)/../include \
    $(QT4CORE_CPPFLAGS) \
    $(GSL_CPPFLAGS) \
    $(RELACS_LIBS_CPPFLAGS)

fishgridrecorder_LDFLAGS = \
    $(QT4CORE_LDFLAGS) \
    $(GSL_LDFLAGS) \
    $(RELACS_LIBS_LDFLAGS)

fishgridrecorder_LDADD = \
    $(QT4CORE_LIBS) \
    $(GSL_LIBS) \
    $(RELACS_LIBS_LIBS)

if FISHGRID_COND_COMEDI
fishgridrecorder_CPPFLAGS += $(COMEDI_CPPFLAGS) 
fishgridrecorder_LDFLAGS += $(COMEDI_LDFLAGS)
fishgridrecorder_LDADD += $(COMEDI_LIBS)
endif

if FISHGRID_COND_NIDAQMX
fishgridrecorder_CPPFLAGS += $(FISHGRID_NIDAQMXBASE_CPPFLAGS) 
fishgridrecorder_LDFLAGS += $(FISHGRID_NIDAQMXBASE_LDFLAGS)
fishgridrecorder_LDADD += $(FISHGRID_NIDAQMXBASE_LIBS)
endif

FISHGRIDRECORDER_MOCFILES =

$(fishgridrecorder_OBJECTS) : ${FISHGRIDRECORDER_MOCFILES}

fishgridrecorder_SOURCES = \
    fishgridrecorder.cc \
    configdata.cc ../include/configdata.h \
    recorder.cc ../include/recorder.h \
    recording.cc ../include/recording.h \
    datathread.cc ../include/datathread.h \
    simulationthread.cc ../include/simulationthread.h \
    ../include/cyclicbuffer.h
if FISHGRID_COND_COMEDI
fishgridrecorder_SOURCES += \
    comedithread.cc ../include/comedithread.h
endif
if FISHGRID_COND_NIDAQMX
fishgridrecorder_SOURCES += \
    nidaqmxthread.cc ../include/nidaqmxthread.h
endif



if FISHGRID_COND_COMEDI

fishgridcalibcomedi_CPPFLAGS = \
    -I$(srcdir)/../include \
    $(GSL_CPPFLAGS) \
    $(RELACS_LIBS_CPPFLAGS) \
    $(COMEDI_CPPFLAGS)

fishgridcalibcomedi_LDFLAGS = \
    $(GSL_LDFLAGS) \
    $(RELACS_LIBS_LDFLAGS) \
    $(COMEDI_LDFLAGS)

fishgridcalibcomedi_LDADD = \
    $(GSL_LIBS) \
    $(RELACS_LIBS_LIBS) \
    $(COMEDI_LIBS)

fishgridcalibcomedi_SOURCES = \
    fishgridcalibcomedi.cc

endif

