#####################################################################
#### Please don't change this file. Use component.mk instead ####
#####################################################################

ifndef SMING_HOME
$(error SMING_HOME is not set: please configure it as an environment variable)
endif

SMING_ARCH := Esp32

include $(SMING_HOME)/project.mk
