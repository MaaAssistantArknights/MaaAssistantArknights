#ifndef __MAIN_H__
#define __MAIN_H__
#ifndef __CLIB__

#include "getopt.h"

static const struct option long_options[] = {
        {"models",         required_argument, NULL, 'd'},
        {"det",            required_argument, NULL, '1'},
        {"cls",            required_argument, NULL, '2'},
        {"rec",            required_argument, NULL, '3'},
        {"keys",           required_argument, NULL, '4'},
        {"image",          required_argument, NULL, 'i'},
        {"numThread",      required_argument, NULL, 't'},
        {"padding",        required_argument, NULL, 'p'},
        {"maxSideLen",     required_argument, NULL, 's'},
        {"boxScoreThresh", required_argument, NULL, 'b'},
        {"boxThresh",      required_argument, NULL, 'o'},
        {"unClipRatio",    required_argument, NULL, 'u'},
        {"doAngle",        required_argument, NULL, 'a'},
        {"mostAngle",      required_argument, NULL, 'A'},
        {"version",        no_argument,       NULL, 'v'},
        {"help",           no_argument,       NULL, 'h'},
        {"loopCount",      required_argument, NULL, 'l'},
        {NULL,             no_argument,       NULL, 0}
};

const char *usageMsg = "(-d --models) (-1 --det) (-2 --cls) (-3 --rec) (-4 --keys) (-i --image)\n"\
                       "[-t --numThread] [-p --padding] [-s --maxSideLen]\n" \
                       "[-b --boxScoreThresh] [-o --boxThresh] [-u --unClipRatio]\n" \
                       "[-a --noAngle] [-A --mostAngle]\n\n";

const char *requiredMsg = "-d --models: models directory.\n" \
                          "-1 --det: model file name of det.\n" \
                          "-2 --cls: model file name of cls.\n" \
                          "-3 --rec: model file name of rec.\n" \
						  "-4 --keys: keys file name.\n" \
                          "-i --image: path of target image.\n\n";

const char *optionalMsg = "-t --numThread: value of numThread(int), default: 4\n" \
                          "-p --padding: value of padding(int), default: 50\n" \
                          "-s --maxSideLen: Long side of picture for resize(int), default: 1024\n" \
                          "-b --boxScoreThresh: value of boxScoreThresh(float), default: 0.6\n" \
                          "-o --boxThresh: value of boxThresh(float), default: 0.3\n" \
                          "-u --unClipRatio: value of unClipRatio(float), default: 2.0\n" \
                          "-a --doAngle: Enable(1)/Disable(0) Angle Net, default: Enable\n" \
                          "-A --mostAngle: Enable(1)/Disable(0) Most Possible AngleIndex, default: Enable\n\n";

const char *otherMsg = "-v --version: show version\n" \
                       "-h --help: print this help\n\n";

const char *example1Msg = "Example1: %s --models models --det det.onnx --cls cls.onnx --rec rec.onnx --keys keys.txt --image 1.jpg\n";
const char *example2Msg = "Example2: %s -d models -1 det.onnx -2 cls.onnx -3 rec.onnx -4 keys.txt -i 1.jpg -t 4 -p 50 -s 0 -b 0.6 -o 0.3 -u 2.0 -a 1 -A 1\n";

#endif
#endif //__MAIN_H__