#!/bin/sh

gcc -I . -fPIC -g -w -fopenmp -std=gnu99 -march=armv6 -mfpu=vfp -mfloat-abi=hard \
    -g3 -ggdb \
    -isystem /opt/vc/include/ \
    -isystem /opt/vc/include/EGL \
    -isystem /opt/vc/include/mmal \
    -isystem /opt/vc/include/interface/vcos/pthreads/ \
    -isystem /opt/vc/include/interface/vmcs_host/linux/ \
    -L /opt/vc/lib -lbcm_host -lmmal -lvcos -lpthread -lmmal_core -lmmal_util  \
   RaspiCLI.c RaspiPreview.c RaspiCamControl.c \
    cameraci.c   cameraci_control.c -o cameraci_control

#videoci.c videoci_control.c -o videoci_control
#
 #raspistill -fp -o test1.jpg
#

if [ $? -eq 0 ] ; then
    sudo ./cameraci_control
fi
