#include "hardcode.hpp"
#include <unordered_map>

namespace glasssix::hardcode
{
    namespace
    {

        struct hardcode_model_params
        {
			inline static const std::vector<std::string> meter_sim{
				"glsv1,"
				"458 589,"
				"Input            images                   0 1 images,0=640 1=640 2=3 3=0,0,0 4=0.0039215686,"
				"MemoryData       /model.80/ia.0/Expand_output_0 0 1 /model.80/ia.0/Expand_output_0 0=80 1=80 2=128,"
				"MemoryData       /model.80/ia.1/Expand_output_0 0 1 /model.80/ia.1/Expand_output_0 0=40 1=40 2=256,"
				"MemoryData       /model.80/ia.2/Expand_output_0 0 1 /model.80/ia.2/Expand_output_0 0=20 1=20 2=512,"
				"MemoryData       /model.80/im.0/Expand_output_0 0 1 /model.80/im.0/Expand_output_0 0=80 1=80 2=24,"
				"MemoryData       /model.80/im.1/Expand_output_0 0 1 /model.80/im.1/Expand_output_0 0=40 1=40 2=24,"
				"MemoryData       /model.80/im.2/Expand_output_0 0 1 /model.80/im.2/Expand_output_0 0=20 1=20 2=24,"
				"Convolution      /model.0/conv/Conv       1 1 images /model.0/conv/Conv_output_0 0=32 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=864,"
				"Split            splitexcalibur_0         1 2 /model.0/conv/Conv_output_0 /model.0/conv/Conv_output_0_splitexcalibur_0 /model.0/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.0/act/Sigmoid     1 1 /model.0/conv/Conv_output_0_splitexcalibur_1 /model.0/act/Sigmoid_output_0,"
				"BinaryOp         /model.0/act/Mul         2 1 /model.0/conv/Conv_output_0_splitexcalibur_0 /model.0/act/Sigmoid_output_0 /model.0/act/Mul_output_0 0=2,"
				"Convolution      /model.1/conv/Conv       1 1 /model.0/act/Mul_output_0 /model.1/conv/Conv_output_0 0=64 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=18432,"
				"Split            splitexcalibur_1         1 2 /model.1/conv/Conv_output_0 /model.1/conv/Conv_output_0_splitexcalibur_0 /model.1/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.1/act/Sigmoid     1 1 /model.1/conv/Conv_output_0_splitexcalibur_1 /model.1/act/Sigmoid_output_0,"
				"BinaryOp         /model.1/act/Mul         2 1 /model.1/conv/Conv_output_0_splitexcalibur_0 /model.1/act/Sigmoid_output_0 /model.1/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_2         1 2 /model.1/act/Mul_output_0 /model.1/act/Mul_output_0_splitexcalibur_0 /model.1/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.2/conv/Conv       1 1 /model.1/act/Mul_output_0_splitexcalibur_1 /model.2/conv/Conv_output_0 0=32 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=2048,"
				"Split            splitexcalibur_3         1 2 /model.2/conv/Conv_output_0 /model.2/conv/Conv_output_0_splitexcalibur_0 /model.2/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.2/act/Sigmoid     1 1 /model.2/conv/Conv_output_0_splitexcalibur_1 /model.2/act/Sigmoid_output_0,"
				"BinaryOp         /model.2/act/Mul         2 1 /model.2/conv/Conv_output_0_splitexcalibur_0 /model.2/act/Sigmoid_output_0 /model.2/act/Mul_output_0 0=2,"
				"Convolution      /model.3/conv/Conv       1 1 /model.1/act/Mul_output_0_splitexcalibur_0 /model.3/conv/Conv_output_0 0=32 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=2048,"
				"Split            splitexcalibur_4         1 2 /model.3/conv/Conv_output_0 /model.3/conv/Conv_output_0_splitexcalibur_0 /model.3/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.3/act/Sigmoid     1 1 /model.3/conv/Conv_output_0_splitexcalibur_1 /model.3/act/Sigmoid_output_0,"
				"BinaryOp         /model.3/act/Mul         2 1 /model.3/conv/Conv_output_0_splitexcalibur_0 /model.3/act/Sigmoid_output_0 /model.3/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_5         1 2 /model.3/act/Mul_output_0 /model.3/act/Mul_output_0_splitexcalibur_0 /model.3/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.4/conv/Conv       1 1 /model.3/act/Mul_output_0_splitexcalibur_1 /model.4/conv/Conv_output_0 0=32 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=9216,"
				"Split            splitexcalibur_6         1 2 /model.4/conv/Conv_output_0 /model.4/conv/Conv_output_0_splitexcalibur_0 /model.4/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.4/act/Sigmoid     1 1 /model.4/conv/Conv_output_0_splitexcalibur_1 /model.4/act/Sigmoid_output_0,"
				"BinaryOp         /model.4/act/Mul         2 1 /model.4/conv/Conv_output_0_splitexcalibur_0 /model.4/act/Sigmoid_output_0 /model.4/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_7         1 2 /model.4/act/Mul_output_0 /model.4/act/Mul_output_0_splitexcalibur_0 /model.4/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.5/conv/Conv       1 1 /model.4/act/Mul_output_0_splitexcalibur_1 /model.5/conv/Conv_output_0 0=32 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=9216,"
				"Split            splitexcalibur_8         1 2 /model.5/conv/Conv_output_0 /model.5/conv/Conv_output_0_splitexcalibur_0 /model.5/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.5/act/Sigmoid     1 1 /model.5/conv/Conv_output_0_splitexcalibur_1 /model.5/act/Sigmoid_output_0,"
				"BinaryOp         /model.5/act/Mul         2 1 /model.5/conv/Conv_output_0_splitexcalibur_0 /model.5/act/Sigmoid_output_0 /model.5/act/Mul_output_0 0=2,"
				"Concat           /model.6/Concat          4 1 /model.5/act/Mul_output_0 /model.4/act/Mul_output_0_splitexcalibur_0 /model.3/act/Mul_output_0_splitexcalibur_0 /model.2/act/Mul_output_0 /model.6/Concat_output_0 0=-1,"
				"Convolution      /model.7/conv/Conv       1 1 /model.6/Concat_output_0 /model.7/conv/Conv_output_0 0=64 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=8192,"
				"Split            splitexcalibur_9         1 2 /model.7/conv/Conv_output_0 /model.7/conv/Conv_output_0_splitexcalibur_0 /model.7/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.7/act/Sigmoid     1 1 /model.7/conv/Conv_output_0_splitexcalibur_1 /model.7/act/Sigmoid_output_0,"
				"BinaryOp         /model.7/act/Mul         2 1 /model.7/conv/Conv_output_0_splitexcalibur_0 /model.7/act/Sigmoid_output_0 /model.7/act/Mul_output_0 0=2,"
				"Convolution      /model.8/conv/Conv       1 1 /model.7/act/Mul_output_0 /model.8/conv/Conv_output_0 0=128 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=73728,"
				"Split            splitexcalibur_10        1 2 /model.8/conv/Conv_output_0 /model.8/conv/Conv_output_0_splitexcalibur_0 /model.8/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.8/act/Sigmoid     1 1 /model.8/conv/Conv_output_0_splitexcalibur_1 /model.8/act/Sigmoid_output_0,"
				"BinaryOp         /model.8/act/Mul         2 1 /model.8/conv/Conv_output_0_splitexcalibur_0 /model.8/act/Sigmoid_output_0 /model.8/act/Mul_output_0 0=2,"
				"Convolution      /model.9/conv/Conv       1 1 /model.8/act/Mul_output_0 /model.9/conv/Conv_output_0 0=64 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=8192,"
				"Split            splitexcalibur_11        1 2 /model.9/conv/Conv_output_0 /model.9/conv/Conv_output_0_splitexcalibur_0 /model.9/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.9/act/Sigmoid     1 1 /model.9/conv/Conv_output_0_splitexcalibur_1 /model.9/act/Sigmoid_output_0,"
				"BinaryOp         /model.9/act/Mul         2 1 /model.9/conv/Conv_output_0_splitexcalibur_0 /model.9/act/Sigmoid_output_0 /model.9/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_12        1 2 /model.9/act/Mul_output_0 /model.9/act/Mul_output_0_splitexcalibur_0 /model.9/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.10/conv/Conv      1 1 /model.9/act/Mul_output_0_splitexcalibur_1 /model.10/conv/Conv_output_0 0=64 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=4096,"
				"Split            splitexcalibur_13        1 2 /model.10/conv/Conv_output_0 /model.10/conv/Conv_output_0_splitexcalibur_0 /model.10/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.10/act/Sigmoid    1 1 /model.10/conv/Conv_output_0_splitexcalibur_1 /model.10/act/Sigmoid_output_0,"
				"BinaryOp         /model.10/act/Mul        2 1 /model.10/conv/Conv_output_0_splitexcalibur_0 /model.10/act/Sigmoid_output_0 /model.10/act/Mul_output_0 0=2,"
				"Convolution      /model.11/conv/Conv      1 1 /model.9/act/Mul_output_0_splitexcalibur_0 /model.11/conv/Conv_output_0 0=64 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=4096,"
				"Split            splitexcalibur_14        1 2 /model.11/conv/Conv_output_0 /model.11/conv/Conv_output_0_splitexcalibur_0 /model.11/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.11/act/Sigmoid    1 1 /model.11/conv/Conv_output_0_splitexcalibur_1 /model.11/act/Sigmoid_output_0,"
				"BinaryOp         /model.11/act/Mul        2 1 /model.11/conv/Conv_output_0_splitexcalibur_0 /model.11/act/Sigmoid_output_0 /model.11/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_15        1 2 /model.11/act/Mul_output_0 /model.11/act/Mul_output_0_splitexcalibur_0 /model.11/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.12/conv/Conv      1 1 /model.11/act/Mul_output_0_splitexcalibur_1 /model.12/conv/Conv_output_0 0=64 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=36864,"
				"Split            splitexcalibur_16        1 2 /model.12/conv/Conv_output_0 /model.12/conv/Conv_output_0_splitexcalibur_0 /model.12/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.12/act/Sigmoid    1 1 /model.12/conv/Conv_output_0_splitexcalibur_1 /model.12/act/Sigmoid_output_0,"
				"BinaryOp         /model.12/act/Mul        2 1 /model.12/conv/Conv_output_0_splitexcalibur_0 /model.12/act/Sigmoid_output_0 /model.12/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_17        1 2 /model.12/act/Mul_output_0 /model.12/act/Mul_output_0_splitexcalibur_0 /model.12/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.13/conv/Conv      1 1 /model.12/act/Mul_output_0_splitexcalibur_1 /model.13/conv/Conv_output_0 0=64 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=36864,"
				"Split            splitexcalibur_18        1 2 /model.13/conv/Conv_output_0 /model.13/conv/Conv_output_0_splitexcalibur_0 /model.13/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.13/act/Sigmoid    1 1 /model.13/conv/Conv_output_0_splitexcalibur_1 /model.13/act/Sigmoid_output_0,"
				"BinaryOp         /model.13/act/Mul        2 1 /model.13/conv/Conv_output_0_splitexcalibur_0 /model.13/act/Sigmoid_output_0 /model.13/act/Mul_output_0 0=2,"
				"Concat           /model.14/Concat         4 1 /model.13/act/Mul_output_0 /model.12/act/Mul_output_0_splitexcalibur_0 /model.11/act/Mul_output_0_splitexcalibur_0 /model.10/act/Mul_output_0 /model.14/Concat_output_0 0=-1,"
				"Convolution      /model.15/conv/Conv      1 1 /model.14/Concat_output_0 /model.15/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=32768,"
				"Split            splitexcalibur_19        1 2 /model.15/conv/Conv_output_0 /model.15/conv/Conv_output_0_splitexcalibur_0 /model.15/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.15/act/Sigmoid    1 1 /model.15/conv/Conv_output_0_splitexcalibur_1 /model.15/act/Sigmoid_output_0,"
				"BinaryOp         /model.15/act/Mul        2 1 /model.15/conv/Conv_output_0_splitexcalibur_0 /model.15/act/Sigmoid_output_0 /model.15/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_20        1 2 /model.15/act/Mul_output_0 /model.15/act/Mul_output_0_splitexcalibur_0 /model.15/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.16/conv/Conv      1 1 /model.15/act/Mul_output_0_splitexcalibur_1 /model.16/conv/Conv_output_0 0=256 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=294912,"
				"Split            splitexcalibur_21        1 2 /model.16/conv/Conv_output_0 /model.16/conv/Conv_output_0_splitexcalibur_0 /model.16/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.16/act/Sigmoid    1 1 /model.16/conv/Conv_output_0_splitexcalibur_1 /model.16/act/Sigmoid_output_0,"
				"BinaryOp         /model.16/act/Mul        2 1 /model.16/conv/Conv_output_0_splitexcalibur_0 /model.16/act/Sigmoid_output_0 /model.16/act/Mul_output_0 0=2,"
				"Convolution      /model.17/conv/Conv      1 1 /model.16/act/Mul_output_0 /model.17/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=32768,"
				"Split            splitexcalibur_22        1 2 /model.17/conv/Conv_output_0 /model.17/conv/Conv_output_0_splitexcalibur_0 /model.17/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.17/act/Sigmoid    1 1 /model.17/conv/Conv_output_0_splitexcalibur_1 /model.17/act/Sigmoid_output_0,"
				"BinaryOp         /model.17/act/Mul        2 1 /model.17/conv/Conv_output_0_splitexcalibur_0 /model.17/act/Sigmoid_output_0 /model.17/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_23        1 2 /model.17/act/Mul_output_0 /model.17/act/Mul_output_0_splitexcalibur_0 /model.17/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.18/conv/Conv      1 1 /model.17/act/Mul_output_0_splitexcalibur_1 /model.18/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=16384,"
				"Split            splitexcalibur_24        1 2 /model.18/conv/Conv_output_0 /model.18/conv/Conv_output_0_splitexcalibur_0 /model.18/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.18/act/Sigmoid    1 1 /model.18/conv/Conv_output_0_splitexcalibur_1 /model.18/act/Sigmoid_output_0,"
				"BinaryOp         /model.18/act/Mul        2 1 /model.18/conv/Conv_output_0_splitexcalibur_0 /model.18/act/Sigmoid_output_0 /model.18/act/Mul_output_0 0=2,"
				"Convolution      /model.19/conv/Conv      1 1 /model.17/act/Mul_output_0_splitexcalibur_0 /model.19/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=16384,"
				"Split            splitexcalibur_25        1 2 /model.19/conv/Conv_output_0 /model.19/conv/Conv_output_0_splitexcalibur_0 /model.19/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.19/act/Sigmoid    1 1 /model.19/conv/Conv_output_0_splitexcalibur_1 /model.19/act/Sigmoid_output_0,"
				"BinaryOp         /model.19/act/Mul        2 1 /model.19/conv/Conv_output_0_splitexcalibur_0 /model.19/act/Sigmoid_output_0 /model.19/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_26        1 2 /model.19/act/Mul_output_0 /model.19/act/Mul_output_0_splitexcalibur_0 /model.19/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.20/conv/Conv      1 1 /model.19/act/Mul_output_0_splitexcalibur_1 /model.20/conv/Conv_output_0 0=128 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=147456,"
				"Split            splitexcalibur_27        1 2 /model.20/conv/Conv_output_0 /model.20/conv/Conv_output_0_splitexcalibur_0 /model.20/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.20/act/Sigmoid    1 1 /model.20/conv/Conv_output_0_splitexcalibur_1 /model.20/act/Sigmoid_output_0,"
				"BinaryOp         /model.20/act/Mul        2 1 /model.20/conv/Conv_output_0_splitexcalibur_0 /model.20/act/Sigmoid_output_0 /model.20/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_28        1 2 /model.20/act/Mul_output_0 /model.20/act/Mul_output_0_splitexcalibur_0 /model.20/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.21/conv/Conv      1 1 /model.20/act/Mul_output_0_splitexcalibur_1 /model.21/conv/Conv_output_0 0=128 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=147456,"
				"Split            splitexcalibur_29        1 2 /model.21/conv/Conv_output_0 /model.21/conv/Conv_output_0_splitexcalibur_0 /model.21/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.21/act/Sigmoid    1 1 /model.21/conv/Conv_output_0_splitexcalibur_1 /model.21/act/Sigmoid_output_0,"
				"BinaryOp         /model.21/act/Mul        2 1 /model.21/conv/Conv_output_0_splitexcalibur_0 /model.21/act/Sigmoid_output_0 /model.21/act/Mul_output_0 0=2,"
				"Concat           /model.22/Concat         4 1 /model.21/act/Mul_output_0 /model.20/act/Mul_output_0_splitexcalibur_0 /model.19/act/Mul_output_0_splitexcalibur_0 /model.18/act/Mul_output_0 /model.22/Concat_output_0 0=-1,"
				"Split            splitexcalibur_30        1 2 /model.22/Concat_output_0 /model.22/Concat_output_0_splitexcalibur_0 /model.22/Concat_output_0_splitexcalibur_1,"
				"Convolution      /model.23/conv/Conv      1 1 /model.22/Concat_output_0_splitexcalibur_1 /model.23/conv/Conv_output_0 0=256 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=131072,"
				"Split            splitexcalibur_31        1 2 /model.23/conv/Conv_output_0 /model.23/conv/Conv_output_0_splitexcalibur_0 /model.23/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.23/act/Sigmoid    1 1 /model.23/conv/Conv_output_0_splitexcalibur_1 /model.23/act/Sigmoid_output_0,"
				"BinaryOp         /model.23/act/Mul        2 1 /model.23/conv/Conv_output_0_splitexcalibur_0 /model.23/act/Sigmoid_output_0 /model.23/act/Mul_output_0 0=2,"
				"Convolution      /model.24/conv/Conv      1 1 /model.23/act/Mul_output_0 /model.24/conv/Conv_output_0 0=512 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=1179648,"
				"Split            splitexcalibur_32        1 2 /model.24/conv/Conv_output_0 /model.24/conv/Conv_output_0_splitexcalibur_0 /model.24/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.24/act/Sigmoid    1 1 /model.24/conv/Conv_output_0_splitexcalibur_1 /model.24/act/Sigmoid_output_0,"
				"BinaryOp         /model.24/act/Mul        2 1 /model.24/conv/Conv_output_0_splitexcalibur_0 /model.24/act/Sigmoid_output_0 /model.24/act/Mul_output_0 0=2,"
				"Convolution      /model.25/conv/Conv      1 1 /model.24/act/Mul_output_0 /model.25/conv/Conv_output_0 0=256 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=131072,"
				"Split            splitexcalibur_33        1 2 /model.25/conv/Conv_output_0 /model.25/conv/Conv_output_0_splitexcalibur_0 /model.25/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.25/act/Sigmoid    1 1 /model.25/conv/Conv_output_0_splitexcalibur_1 /model.25/act/Sigmoid_output_0,"
				"BinaryOp         /model.25/act/Mul        2 1 /model.25/conv/Conv_output_0_splitexcalibur_0 /model.25/act/Sigmoid_output_0 /model.25/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_34        1 2 /model.25/act/Mul_output_0 /model.25/act/Mul_output_0_splitexcalibur_0 /model.25/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.26/conv/Conv      1 1 /model.25/act/Mul_output_0_splitexcalibur_1 /model.26/conv/Conv_output_0 0=256 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=65536,"
				"Split            splitexcalibur_35        1 2 /model.26/conv/Conv_output_0 /model.26/conv/Conv_output_0_splitexcalibur_0 /model.26/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.26/act/Sigmoid    1 1 /model.26/conv/Conv_output_0_splitexcalibur_1 /model.26/act/Sigmoid_output_0,"
				"BinaryOp         /model.26/act/Mul        2 1 /model.26/conv/Conv_output_0_splitexcalibur_0 /model.26/act/Sigmoid_output_0 /model.26/act/Mul_output_0 0=2,"
				"Convolution      /model.27/conv/Conv      1 1 /model.25/act/Mul_output_0_splitexcalibur_0 /model.27/conv/Conv_output_0 0=256 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=65536,"
				"Split            splitexcalibur_36        1 2 /model.27/conv/Conv_output_0 /model.27/conv/Conv_output_0_splitexcalibur_0 /model.27/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.27/act/Sigmoid    1 1 /model.27/conv/Conv_output_0_splitexcalibur_1 /model.27/act/Sigmoid_output_0,"
				"BinaryOp         /model.27/act/Mul        2 1 /model.27/conv/Conv_output_0_splitexcalibur_0 /model.27/act/Sigmoid_output_0 /model.27/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_37        1 2 /model.27/act/Mul_output_0 /model.27/act/Mul_output_0_splitexcalibur_0 /model.27/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.28/conv/Conv      1 1 /model.27/act/Mul_output_0_splitexcalibur_1 /model.28/conv/Conv_output_0 0=256 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=589824,"
				"Split            splitexcalibur_38        1 2 /model.28/conv/Conv_output_0 /model.28/conv/Conv_output_0_splitexcalibur_0 /model.28/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.28/act/Sigmoid    1 1 /model.28/conv/Conv_output_0_splitexcalibur_1 /model.28/act/Sigmoid_output_0,"
				"BinaryOp         /model.28/act/Mul        2 1 /model.28/conv/Conv_output_0_splitexcalibur_0 /model.28/act/Sigmoid_output_0 /model.28/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_39        1 2 /model.28/act/Mul_output_0 /model.28/act/Mul_output_0_splitexcalibur_0 /model.28/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.29/conv/Conv      1 1 /model.28/act/Mul_output_0_splitexcalibur_1 /model.29/conv/Conv_output_0 0=256 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=589824,"
				"Split            splitexcalibur_40        1 2 /model.29/conv/Conv_output_0 /model.29/conv/Conv_output_0_splitexcalibur_0 /model.29/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.29/act/Sigmoid    1 1 /model.29/conv/Conv_output_0_splitexcalibur_1 /model.29/act/Sigmoid_output_0,"
				"BinaryOp         /model.29/act/Mul        2 1 /model.29/conv/Conv_output_0_splitexcalibur_0 /model.29/act/Sigmoid_output_0 /model.29/act/Mul_output_0 0=2,"
				"Concat           /model.30/Concat         4 1 /model.29/act/Mul_output_0 /model.28/act/Mul_output_0_splitexcalibur_0 /model.27/act/Mul_output_0_splitexcalibur_0 /model.26/act/Mul_output_0 /model.30/Concat_output_0 0=-1,"
				"Convolution      /model.31/conv/Conv      1 1 /model.30/Concat_output_0 /model.31/conv/Conv_output_0 0=512 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=524288,"
				"Split            splitexcalibur_41        1 2 /model.31/conv/Conv_output_0 /model.31/conv/Conv_output_0_splitexcalibur_0 /model.31/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.31/act/Sigmoid    1 1 /model.31/conv/Conv_output_0_splitexcalibur_1 /model.31/act/Sigmoid_output_0,"
				"BinaryOp         /model.31/act/Mul        2 1 /model.31/conv/Conv_output_0_splitexcalibur_0 /model.31/act/Sigmoid_output_0 /model.31/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_42        1 2 /model.31/act/Mul_output_0 /model.31/act/Mul_output_0_splitexcalibur_0 /model.31/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.32/conv/Conv      1 1 /model.31/act/Mul_output_0_splitexcalibur_1 /model.32/conv/Conv_output_0 0=256 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=131072,"
				"Split            splitexcalibur_43        1 2 /model.32/conv/Conv_output_0 /model.32/conv/Conv_output_0_splitexcalibur_0 /model.32/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.32/act/Sigmoid    1 1 /model.32/conv/Conv_output_0_splitexcalibur_1 /model.32/act/Sigmoid_output_0,"
				"BinaryOp         /model.32/act/Mul        2 1 /model.32/conv/Conv_output_0_splitexcalibur_0 /model.32/act/Sigmoid_output_0 /model.32/act/Mul_output_0 0=2,"
				"Convolution      /model.33/conv/Conv      1 1 /model.31/act/Mul_output_0_splitexcalibur_0 /model.33/conv/Conv_output_0 0=256 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=131072,"
				"Split            splitexcalibur_44        1 2 /model.33/conv/Conv_output_0 /model.33/conv/Conv_output_0_splitexcalibur_0 /model.33/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.33/act/Sigmoid    1 1 /model.33/conv/Conv_output_0_splitexcalibur_1 /model.33/act/Sigmoid_output_0,"
				"BinaryOp         /model.33/act/Mul        2 1 /model.33/conv/Conv_output_0_splitexcalibur_0 /model.33/act/Sigmoid_output_0 /model.33/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_45        1 4 /model.33/act/Mul_output_0 /model.33/act/Mul_output_0_splitexcalibur_0 /model.33/act/Mul_output_0_splitexcalibur_1 /model.33/act/Mul_output_0_splitexcalibur_2 /model.33/act/Mul_output_0_splitexcalibur_3,"
				"Pooling          /model.34/m/MaxPool      1 1 /model.33/act/Mul_output_0_splitexcalibur_3 /model.34/m/MaxPool_output_0 0=0 1=5 11=5 2=1 12=1 3=2 13=2 14=2 15=2 5=1,"
				"Pooling          /model.35/m/MaxPool      1 1 /model.33/act/Mul_output_0_splitexcalibur_2 /model.35/m/MaxPool_output_0 0=0 1=9 11=9 2=1 12=1 3=4 13=4 14=4 15=4 5=1,"
				"Pooling          /model.36/m/MaxPool      1 1 /model.33/act/Mul_output_0_splitexcalibur_1 /model.36/m/MaxPool_output_0 0=0 1=13 11=13 2=1 12=1 3=6 13=6 14=6 15=6 5=1,"
				"Concat           /model.37/Concat         4 1 /model.36/m/MaxPool_output_0 /model.35/m/MaxPool_output_0 /model.34/m/MaxPool_output_0 /model.33/act/Mul_output_0_splitexcalibur_0 /model.37/Concat_output_0 0=-1,"
				"Convolution      /model.38/conv/Conv      1 1 /model.37/Concat_output_0 /model.38/conv/Conv_output_0 0=256 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=262144,"
				"Split            splitexcalibur_46        1 2 /model.38/conv/Conv_output_0 /model.38/conv/Conv_output_0_splitexcalibur_0 /model.38/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.38/act/Sigmoid    1 1 /model.38/conv/Conv_output_0_splitexcalibur_1 /model.38/act/Sigmoid_output_0,"
				"BinaryOp         /model.38/act/Mul        2 1 /model.38/conv/Conv_output_0_splitexcalibur_0 /model.38/act/Sigmoid_output_0 /model.38/act/Mul_output_0 0=2,"
				"Concat           /model.39/Concat         2 1 /model.38/act/Mul_output_0 /model.32/act/Mul_output_0 /model.39/Concat_output_0 0=-1,"
				"Convolution      /model.40/conv/Conv      1 1 /model.39/Concat_output_0 /model.40/conv/Conv_output_0 0=256 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=131072,"
				"Split            splitexcalibur_47        1 2 /model.40/conv/Conv_output_0 /model.40/conv/Conv_output_0_splitexcalibur_0 /model.40/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.40/act/Sigmoid    1 1 /model.40/conv/Conv_output_0_splitexcalibur_1 /model.40/act/Sigmoid_output_0,"
				"BinaryOp         /model.40/act/Mul        2 1 /model.40/conv/Conv_output_0_splitexcalibur_0 /model.40/act/Sigmoid_output_0 /model.40/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_48        1 2 /model.40/act/Mul_output_0 /model.40/act/Mul_output_0_splitexcalibur_0 /model.40/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.41/conv/Conv      1 1 /model.40/act/Mul_output_0_splitexcalibur_1 /model.41/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=32768,"
				"Split            splitexcalibur_49        1 2 /model.41/conv/Conv_output_0 /model.41/conv/Conv_output_0_splitexcalibur_0 /model.41/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.41/act/Sigmoid    1 1 /model.41/conv/Conv_output_0_splitexcalibur_1 /model.41/act/Sigmoid_output_0,"
				"BinaryOp         /model.41/act/Mul        2 1 /model.41/conv/Conv_output_0_splitexcalibur_0 /model.41/act/Sigmoid_output_0 /model.41/act/Mul_output_0 0=2,"
				"Interp           /model.42/Resize         1 1 /model.41/act/Mul_output_0 /model.42/Resize_output_0 0=1 1=2.000000e+00 2=2.000000e+00 3=0 4=0 6=0,"
				"Convolution      /model.43/conv/Conv      1 1 /model.22/Concat_output_0_splitexcalibur_0 /model.43/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=65536,"
				"Split            splitexcalibur_50        1 2 /model.43/conv/Conv_output_0 /model.43/conv/Conv_output_0_splitexcalibur_0 /model.43/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.43/act/Sigmoid    1 1 /model.43/conv/Conv_output_0_splitexcalibur_1 /model.43/act/Sigmoid_output_0,"
				"BinaryOp         /model.43/act/Mul        2 1 /model.43/conv/Conv_output_0_splitexcalibur_0 /model.43/act/Sigmoid_output_0 /model.43/act/Mul_output_0 0=2,"
				"Concat           /model.44/Concat         2 1 /model.43/act/Mul_output_0 /model.42/Resize_output_0 /model.44/Concat_output_0 0=-1,"
				"Split            splitexcalibur_51        1 2 /model.44/Concat_output_0 /model.44/Concat_output_0_splitexcalibur_0 /model.44/Concat_output_0_splitexcalibur_1,"
				"Convolution      /model.45/conv/Conv      1 1 /model.44/Concat_output_0_splitexcalibur_1 /model.45/conv/Conv_output_0 0=64 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=16384,"
				"Split            splitexcalibur_52        1 2 /model.45/conv/Conv_output_0 /model.45/conv/Conv_output_0_splitexcalibur_0 /model.45/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.45/act/Sigmoid    1 1 /model.45/conv/Conv_output_0_splitexcalibur_1 /model.45/act/Sigmoid_output_0,"
				"BinaryOp         /model.45/act/Mul        2 1 /model.45/conv/Conv_output_0_splitexcalibur_0 /model.45/act/Sigmoid_output_0 /model.45/act/Mul_output_0 0=2,"
				"Convolution      /model.46/conv/Conv      1 1 /model.44/Concat_output_0_splitexcalibur_0 /model.46/conv/Conv_output_0 0=64 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=16384,"
				"Split            splitexcalibur_53        1 2 /model.46/conv/Conv_output_0 /model.46/conv/Conv_output_0_splitexcalibur_0 /model.46/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.46/act/Sigmoid    1 1 /model.46/conv/Conv_output_0_splitexcalibur_1 /model.46/act/Sigmoid_output_0,"
				"BinaryOp         /model.46/act/Mul        2 1 /model.46/conv/Conv_output_0_splitexcalibur_0 /model.46/act/Sigmoid_output_0 /model.46/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_54        1 2 /model.46/act/Mul_output_0 /model.46/act/Mul_output_0_splitexcalibur_0 /model.46/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.47/conv/Conv      1 1 /model.46/act/Mul_output_0_splitexcalibur_1 /model.47/conv/Conv_output_0 0=64 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=36864,"
				"Split            splitexcalibur_55        1 2 /model.47/conv/Conv_output_0 /model.47/conv/Conv_output_0_splitexcalibur_0 /model.47/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.47/act/Sigmoid    1 1 /model.47/conv/Conv_output_0_splitexcalibur_1 /model.47/act/Sigmoid_output_0,"
				"BinaryOp         /model.47/act/Mul        2 1 /model.47/conv/Conv_output_0_splitexcalibur_0 /model.47/act/Sigmoid_output_0 /model.47/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_56        1 2 /model.47/act/Mul_output_0 /model.47/act/Mul_output_0_splitexcalibur_0 /model.47/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.48/conv/Conv      1 1 /model.47/act/Mul_output_0_splitexcalibur_1 /model.48/conv/Conv_output_0 0=64 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=36864,"
				"Split            splitexcalibur_57        1 2 /model.48/conv/Conv_output_0 /model.48/conv/Conv_output_0_splitexcalibur_0 /model.48/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.48/act/Sigmoid    1 1 /model.48/conv/Conv_output_0_splitexcalibur_1 /model.48/act/Sigmoid_output_0,"
				"BinaryOp         /model.48/act/Mul        2 1 /model.48/conv/Conv_output_0_splitexcalibur_0 /model.48/act/Sigmoid_output_0 /model.48/act/Mul_output_0 0=2,"
				"Concat           /model.49/Concat         4 1 /model.48/act/Mul_output_0 /model.47/act/Mul_output_0_splitexcalibur_0 /model.46/act/Mul_output_0_splitexcalibur_0 /model.45/act/Mul_output_0 /model.49/Concat_output_0 0=-1,"
				"Convolution      /model.50/conv/Conv      1 1 /model.49/Concat_output_0 /model.50/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=32768,"
				"Split            splitexcalibur_58        1 2 /model.50/conv/Conv_output_0 /model.50/conv/Conv_output_0_splitexcalibur_0 /model.50/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.50/act/Sigmoid    1 1 /model.50/conv/Conv_output_0_splitexcalibur_1 /model.50/act/Sigmoid_output_0,"
				"BinaryOp         /model.50/act/Mul        2 1 /model.50/conv/Conv_output_0_splitexcalibur_0 /model.50/act/Sigmoid_output_0 /model.50/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_59        1 2 /model.50/act/Mul_output_0 /model.50/act/Mul_output_0_splitexcalibur_0 /model.50/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.51/conv/Conv      1 1 /model.50/act/Mul_output_0_splitexcalibur_1 /model.51/conv/Conv_output_0 0=64 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=8192,"
				"Split            splitexcalibur_60        1 2 /model.51/conv/Conv_output_0 /model.51/conv/Conv_output_0_splitexcalibur_0 /model.51/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.51/act/Sigmoid    1 1 /model.51/conv/Conv_output_0_splitexcalibur_1 /model.51/act/Sigmoid_output_0,"
				"BinaryOp         /model.51/act/Mul        2 1 /model.51/conv/Conv_output_0_splitexcalibur_0 /model.51/act/Sigmoid_output_0 /model.51/act/Mul_output_0 0=2,"
				"Interp           /model.52/Resize         1 1 /model.51/act/Mul_output_0 /model.52/Resize_output_0 0=1 1=2.000000e+00 2=2.000000e+00 3=0 4=0 6=0,"
				"Convolution      /model.53/conv/Conv      1 1 /model.15/act/Mul_output_0_splitexcalibur_0 /model.53/conv/Conv_output_0 0=64 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=8192,"
				"Split            splitexcalibur_61        1 2 /model.53/conv/Conv_output_0 /model.53/conv/Conv_output_0_splitexcalibur_0 /model.53/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.53/act/Sigmoid    1 1 /model.53/conv/Conv_output_0_splitexcalibur_1 /model.53/act/Sigmoid_output_0,"
				"BinaryOp         /model.53/act/Mul        2 1 /model.53/conv/Conv_output_0_splitexcalibur_0 /model.53/act/Sigmoid_output_0 /model.53/act/Mul_output_0 0=2,"
				"Concat           /model.54/Concat         2 1 /model.53/act/Mul_output_0 /model.52/Resize_output_0 /model.54/Concat_output_0 0=-1,"
				"Split            splitexcalibur_62        1 2 /model.54/Concat_output_0 /model.54/Concat_output_0_splitexcalibur_0 /model.54/Concat_output_0_splitexcalibur_1,"
				"Convolution      /model.55/conv/Conv      1 1 /model.54/Concat_output_0_splitexcalibur_1 /model.55/conv/Conv_output_0 0=32 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=4096,"
				"Split            splitexcalibur_63        1 2 /model.55/conv/Conv_output_0 /model.55/conv/Conv_output_0_splitexcalibur_0 /model.55/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.55/act/Sigmoid    1 1 /model.55/conv/Conv_output_0_splitexcalibur_1 /model.55/act/Sigmoid_output_0,"
				"BinaryOp         /model.55/act/Mul        2 1 /model.55/conv/Conv_output_0_splitexcalibur_0 /model.55/act/Sigmoid_output_0 /model.55/act/Mul_output_0 0=2,"
				"Convolution      /model.56/conv/Conv      1 1 /model.54/Concat_output_0_splitexcalibur_0 /model.56/conv/Conv_output_0 0=32 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=4096,"
				"Split            splitexcalibur_64        1 2 /model.56/conv/Conv_output_0 /model.56/conv/Conv_output_0_splitexcalibur_0 /model.56/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.56/act/Sigmoid    1 1 /model.56/conv/Conv_output_0_splitexcalibur_1 /model.56/act/Sigmoid_output_0,"
				"BinaryOp         /model.56/act/Mul        2 1 /model.56/conv/Conv_output_0_splitexcalibur_0 /model.56/act/Sigmoid_output_0 /model.56/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_65        1 2 /model.56/act/Mul_output_0 /model.56/act/Mul_output_0_splitexcalibur_0 /model.56/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.57/conv/Conv      1 1 /model.56/act/Mul_output_0_splitexcalibur_1 /model.57/conv/Conv_output_0 0=32 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=9216,"
				"Split            splitexcalibur_66        1 2 /model.57/conv/Conv_output_0 /model.57/conv/Conv_output_0_splitexcalibur_0 /model.57/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.57/act/Sigmoid    1 1 /model.57/conv/Conv_output_0_splitexcalibur_1 /model.57/act/Sigmoid_output_0,"
				"BinaryOp         /model.57/act/Mul        2 1 /model.57/conv/Conv_output_0_splitexcalibur_0 /model.57/act/Sigmoid_output_0 /model.57/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_67        1 2 /model.57/act/Mul_output_0 /model.57/act/Mul_output_0_splitexcalibur_0 /model.57/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.58/conv/Conv      1 1 /model.57/act/Mul_output_0_splitexcalibur_1 /model.58/conv/Conv_output_0 0=32 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=9216,"
				"Split            splitexcalibur_68        1 2 /model.58/conv/Conv_output_0 /model.58/conv/Conv_output_0_splitexcalibur_0 /model.58/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.58/act/Sigmoid    1 1 /model.58/conv/Conv_output_0_splitexcalibur_1 /model.58/act/Sigmoid_output_0,"
				"BinaryOp         /model.58/act/Mul        2 1 /model.58/conv/Conv_output_0_splitexcalibur_0 /model.58/act/Sigmoid_output_0 /model.58/act/Mul_output_0 0=2,"
				"Concat           /model.59/Concat         4 1 /model.58/act/Mul_output_0 /model.57/act/Mul_output_0_splitexcalibur_0 /model.56/act/Mul_output_0_splitexcalibur_0 /model.55/act/Mul_output_0 /model.59/Concat_output_0 0=-1,"
				"Convolution      /model.60/conv/Conv      1 1 /model.59/Concat_output_0 /model.60/conv/Conv_output_0 0=64 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=8192,"
				"Split            splitexcalibur_69        1 2 /model.60/conv/Conv_output_0 /model.60/conv/Conv_output_0_splitexcalibur_0 /model.60/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.60/act/Sigmoid    1 1 /model.60/conv/Conv_output_0_splitexcalibur_1 /model.60/act/Sigmoid_output_0,"
				"BinaryOp         /model.60/act/Mul        2 1 /model.60/conv/Conv_output_0_splitexcalibur_0 /model.60/act/Sigmoid_output_0 /model.60/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_70        1 2 /model.60/act/Mul_output_0 /model.60/act/Mul_output_0_splitexcalibur_0 /model.60/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.61/conv/Conv      1 1 /model.60/act/Mul_output_0_splitexcalibur_1 /model.61/conv/Conv_output_0 0=128 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=73728,"
				"Split            splitexcalibur_71        1 2 /model.61/conv/Conv_output_0 /model.61/conv/Conv_output_0_splitexcalibur_0 /model.61/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.61/act/Sigmoid    1 1 /model.61/conv/Conv_output_0_splitexcalibur_1 /model.61/act/Sigmoid_output_0,"
				"BinaryOp         /model.61/act/Mul        2 1 /model.61/conv/Conv_output_0_splitexcalibur_0 /model.61/act/Sigmoid_output_0 /model.61/act/Mul_output_0 0=2,"
				"Concat           /model.62/Concat         2 1 /model.61/act/Mul_output_0 /model.50/act/Mul_output_0_splitexcalibur_0 /model.62/Concat_output_0 0=-1,"
				"Split            splitexcalibur_72        1 2 /model.62/Concat_output_0 /model.62/Concat_output_0_splitexcalibur_0 /model.62/Concat_output_0_splitexcalibur_1,"
				"Convolution      /model.63/conv/Conv      1 1 /model.62/Concat_output_0_splitexcalibur_1 /model.63/conv/Conv_output_0 0=64 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=16384,"
				"Split            splitexcalibur_73        1 2 /model.63/conv/Conv_output_0 /model.63/conv/Conv_output_0_splitexcalibur_0 /model.63/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.63/act/Sigmoid    1 1 /model.63/conv/Conv_output_0_splitexcalibur_1 /model.63/act/Sigmoid_output_0,"
				"BinaryOp         /model.63/act/Mul        2 1 /model.63/conv/Conv_output_0_splitexcalibur_0 /model.63/act/Sigmoid_output_0 /model.63/act/Mul_output_0 0=2,"
				"Convolution      /model.64/conv/Conv      1 1 /model.62/Concat_output_0_splitexcalibur_0 /model.64/conv/Conv_output_0 0=64 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=16384,"
				"Split            splitexcalibur_74        1 2 /model.64/conv/Conv_output_0 /model.64/conv/Conv_output_0_splitexcalibur_0 /model.64/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.64/act/Sigmoid    1 1 /model.64/conv/Conv_output_0_splitexcalibur_1 /model.64/act/Sigmoid_output_0,"
				"BinaryOp         /model.64/act/Mul        2 1 /model.64/conv/Conv_output_0_splitexcalibur_0 /model.64/act/Sigmoid_output_0 /model.64/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_75        1 2 /model.64/act/Mul_output_0 /model.64/act/Mul_output_0_splitexcalibur_0 /model.64/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.65/conv/Conv      1 1 /model.64/act/Mul_output_0_splitexcalibur_1 /model.65/conv/Conv_output_0 0=64 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=36864,"
				"Split            splitexcalibur_76        1 2 /model.65/conv/Conv_output_0 /model.65/conv/Conv_output_0_splitexcalibur_0 /model.65/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.65/act/Sigmoid    1 1 /model.65/conv/Conv_output_0_splitexcalibur_1 /model.65/act/Sigmoid_output_0,"
				"BinaryOp         /model.65/act/Mul        2 1 /model.65/conv/Conv_output_0_splitexcalibur_0 /model.65/act/Sigmoid_output_0 /model.65/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_77        1 2 /model.65/act/Mul_output_0 /model.65/act/Mul_output_0_splitexcalibur_0 /model.65/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.66/conv/Conv      1 1 /model.65/act/Mul_output_0_splitexcalibur_1 /model.66/conv/Conv_output_0 0=64 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=36864,"
				"Split            splitexcalibur_78        1 2 /model.66/conv/Conv_output_0 /model.66/conv/Conv_output_0_splitexcalibur_0 /model.66/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.66/act/Sigmoid    1 1 /model.66/conv/Conv_output_0_splitexcalibur_1 /model.66/act/Sigmoid_output_0,"
				"BinaryOp         /model.66/act/Mul        2 1 /model.66/conv/Conv_output_0_splitexcalibur_0 /model.66/act/Sigmoid_output_0 /model.66/act/Mul_output_0 0=2,"
				"Concat           /model.67/Concat         4 1 /model.66/act/Mul_output_0 /model.65/act/Mul_output_0_splitexcalibur_0 /model.64/act/Mul_output_0_splitexcalibur_0 /model.63/act/Mul_output_0 /model.67/Concat_output_0 0=-1,"
				"Convolution      /model.68/conv/Conv      1 1 /model.67/Concat_output_0 /model.68/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=32768,"
				"Split            splitexcalibur_79        1 2 /model.68/conv/Conv_output_0 /model.68/conv/Conv_output_0_splitexcalibur_0 /model.68/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.68/act/Sigmoid    1 1 /model.68/conv/Conv_output_0_splitexcalibur_1 /model.68/act/Sigmoid_output_0,"
				"BinaryOp         /model.68/act/Mul        2 1 /model.68/conv/Conv_output_0_splitexcalibur_0 /model.68/act/Sigmoid_output_0 /model.68/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_80        1 2 /model.68/act/Mul_output_0 /model.68/act/Mul_output_0_splitexcalibur_0 /model.68/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.69/conv/Conv      1 1 /model.68/act/Mul_output_0_splitexcalibur_1 /model.69/conv/Conv_output_0 0=256 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=294912,"
				"Split            splitexcalibur_81        1 2 /model.69/conv/Conv_output_0 /model.69/conv/Conv_output_0_splitexcalibur_0 /model.69/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.69/act/Sigmoid    1 1 /model.69/conv/Conv_output_0_splitexcalibur_1 /model.69/act/Sigmoid_output_0,"
				"BinaryOp         /model.69/act/Mul        2 1 /model.69/conv/Conv_output_0_splitexcalibur_0 /model.69/act/Sigmoid_output_0 /model.69/act/Mul_output_0 0=2,"
				"Concat           /model.70/Concat         2 1 /model.69/act/Mul_output_0 /model.40/act/Mul_output_0_splitexcalibur_0 /model.70/Concat_output_0 0=-1,"
				"Split            splitexcalibur_82        1 2 /model.70/Concat_output_0 /model.70/Concat_output_0_splitexcalibur_0 /model.70/Concat_output_0_splitexcalibur_1,"
				"Convolution      /model.71/conv/Conv      1 1 /model.70/Concat_output_0_splitexcalibur_1 /model.71/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=65536,"
				"Split            splitexcalibur_83        1 2 /model.71/conv/Conv_output_0 /model.71/conv/Conv_output_0_splitexcalibur_0 /model.71/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.71/act/Sigmoid    1 1 /model.71/conv/Conv_output_0_splitexcalibur_1 /model.71/act/Sigmoid_output_0,"
				"BinaryOp         /model.71/act/Mul        2 1 /model.71/conv/Conv_output_0_splitexcalibur_0 /model.71/act/Sigmoid_output_0 /model.71/act/Mul_output_0 0=2,"
				"Convolution      /model.72/conv/Conv      1 1 /model.70/Concat_output_0_splitexcalibur_0 /model.72/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=65536,"
				"Split            splitexcalibur_84        1 2 /model.72/conv/Conv_output_0 /model.72/conv/Conv_output_0_splitexcalibur_0 /model.72/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.72/act/Sigmoid    1 1 /model.72/conv/Conv_output_0_splitexcalibur_1 /model.72/act/Sigmoid_output_0,"
				"BinaryOp         /model.72/act/Mul        2 1 /model.72/conv/Conv_output_0_splitexcalibur_0 /model.72/act/Sigmoid_output_0 /model.72/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_85        1 2 /model.72/act/Mul_output_0 /model.72/act/Mul_output_0_splitexcalibur_0 /model.72/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.73/conv/Conv      1 1 /model.72/act/Mul_output_0_splitexcalibur_1 /model.73/conv/Conv_output_0 0=128 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=147456,"
				"Split            splitexcalibur_86        1 2 /model.73/conv/Conv_output_0 /model.73/conv/Conv_output_0_splitexcalibur_0 /model.73/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.73/act/Sigmoid    1 1 /model.73/conv/Conv_output_0_splitexcalibur_1 /model.73/act/Sigmoid_output_0,"
				"BinaryOp         /model.73/act/Mul        2 1 /model.73/conv/Conv_output_0_splitexcalibur_0 /model.73/act/Sigmoid_output_0 /model.73/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_87        1 2 /model.73/act/Mul_output_0 /model.73/act/Mul_output_0_splitexcalibur_0 /model.73/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.74/conv/Conv      1 1 /model.73/act/Mul_output_0_splitexcalibur_1 /model.74/conv/Conv_output_0 0=128 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=147456,"
				"Split            splitexcalibur_88        1 2 /model.74/conv/Conv_output_0 /model.74/conv/Conv_output_0_splitexcalibur_0 /model.74/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.74/act/Sigmoid    1 1 /model.74/conv/Conv_output_0_splitexcalibur_1 /model.74/act/Sigmoid_output_0,"
				"BinaryOp         /model.74/act/Mul        2 1 /model.74/conv/Conv_output_0_splitexcalibur_0 /model.74/act/Sigmoid_output_0 /model.74/act/Mul_output_0 0=2,"
				"Concat           /model.75/Concat         4 1 /model.74/act/Mul_output_0 /model.73/act/Mul_output_0_splitexcalibur_0 /model.72/act/Mul_output_0_splitexcalibur_0 /model.71/act/Mul_output_0 /model.75/Concat_output_0 0=-1,"
				"Convolution      /model.76/conv/Conv      1 1 /model.75/Concat_output_0 /model.76/conv/Conv_output_0 0=256 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=131072,"
				"Split            splitexcalibur_89        1 2 /model.76/conv/Conv_output_0 /model.76/conv/Conv_output_0_splitexcalibur_0 /model.76/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.76/act/Sigmoid    1 1 /model.76/conv/Conv_output_0_splitexcalibur_1 /model.76/act/Sigmoid_output_0,"
				"BinaryOp         /model.76/act/Mul        2 1 /model.76/conv/Conv_output_0_splitexcalibur_0 /model.76/act/Sigmoid_output_0 /model.76/act/Mul_output_0 0=2,"
				"Convolution      /model.77/conv/Conv      1 1 /model.60/act/Mul_output_0_splitexcalibur_0 /model.77/conv/Conv_output_0 0=128 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=73728,"
				"Split            splitexcalibur_90        1 2 /model.77/conv/Conv_output_0 /model.77/conv/Conv_output_0_splitexcalibur_0 /model.77/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.77/act/Sigmoid    1 1 /model.77/conv/Conv_output_0_splitexcalibur_1 /model.77/act/Sigmoid_output_0,"
				"BinaryOp         /model.77/act/Mul        2 1 /model.77/conv/Conv_output_0_splitexcalibur_0 /model.77/act/Sigmoid_output_0 /model.77/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_91        1 2 /model.77/act/Mul_output_0 /model.77/act/Mul_output_0_splitexcalibur_0 /model.77/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.78/conv/Conv      1 1 /model.68/act/Mul_output_0_splitexcalibur_0 /model.78/conv/Conv_output_0 0=256 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=294912,"
				"Split            splitexcalibur_92        1 2 /model.78/conv/Conv_output_0 /model.78/conv/Conv_output_0_splitexcalibur_0 /model.78/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.78/act/Sigmoid    1 1 /model.78/conv/Conv_output_0_splitexcalibur_1 /model.78/act/Sigmoid_output_0,"
				"BinaryOp         /model.78/act/Mul        2 1 /model.78/conv/Conv_output_0_splitexcalibur_0 /model.78/act/Sigmoid_output_0 /model.78/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_93        1 2 /model.78/act/Mul_output_0 /model.78/act/Mul_output_0_splitexcalibur_0 /model.78/act/Mul_output_0_splitexcalibur_1,"
				"Convolution      /model.79/conv/Conv      1 1 /model.76/act/Mul_output_0 /model.79/conv/Conv_output_0 0=512 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=1179648,"
				"Split            splitexcalibur_94        1 2 /model.79/conv/Conv_output_0 /model.79/conv/Conv_output_0_splitexcalibur_0 /model.79/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.79/act/Sigmoid    1 1 /model.79/conv/Conv_output_0_splitexcalibur_1 /model.79/act/Sigmoid_output_0,"
				"BinaryOp         /model.79/act/Mul        2 1 /model.79/conv/Conv_output_0_splitexcalibur_0 /model.79/act/Sigmoid_output_0 /model.79/act/Mul_output_0 0=2,"
				"Split            splitexcalibur_95        1 2 /model.79/act/Mul_output_0 /model.79/act/Mul_output_0_splitexcalibur_0 /model.79/act/Mul_output_0_splitexcalibur_1,"
				"BinaryOp         /model.80/ia.0/Add       2 1 /model.80/ia.0/Expand_output_0 /model.77/act/Mul_output_0_splitexcalibur_1 /model.80/ia.0/Add_output_0 0=0,"
				"Convolution      /model.80/m.0/Conv       1 1 /model.80/ia.0/Add_output_0 /model.80/m.0/Conv_output_0 0=24 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=3072,"
				"BinaryOp         /model.80/im.0/Mul       2 1 /model.80/im.0/Expand_output_0 /model.80/m.0/Conv_output_0 /model.80/im.0/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.0/m_kpt.0.0/conv/Conv 1 1 /model.77/act/Mul_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.0/conv/Conv_output_0 0=128 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=1152 7=128,"
				"Split            splitexcalibur_96        1 2 /model.80/m_kpt.0/m_kpt.0.0/conv/Conv_output_0 /model.80/m_kpt.0/m_kpt.0.0/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.0/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.0/m_kpt.0.0/act/Sigmoid 1 1 /model.80/m_kpt.0/m_kpt.0.0/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.0/m_kpt.0.0/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.0/m_kpt.0.0/act/Mul 2 1 /model.80/m_kpt.0/m_kpt.0.0/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.0/act/Sigmoid_output_0 /model.80/m_kpt.0/m_kpt.0.0/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.0/m_kpt.0.1/conv/Conv 1 1 /model.80/m_kpt.0/m_kpt.0.0/act/Mul_output_0 /model.80/m_kpt.0/m_kpt.0.1/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=16384,"
				"Split            splitexcalibur_97        1 2 /model.80/m_kpt.0/m_kpt.0.1/conv/Conv_output_0 /model.80/m_kpt.0/m_kpt.0.1/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.1/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.0/m_kpt.0.1/act/Sigmoid 1 1 /model.80/m_kpt.0/m_kpt.0.1/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.0/m_kpt.0.1/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.0/m_kpt.0.1/act/Mul 2 1 /model.80/m_kpt.0/m_kpt.0.1/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.1/act/Sigmoid_output_0 /model.80/m_kpt.0/m_kpt.0.1/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.0/m_kpt.0.2/conv/Conv 1 1 /model.80/m_kpt.0/m_kpt.0.1/act/Mul_output_0 /model.80/m_kpt.0/m_kpt.0.2/conv/Conv_output_0 0=128 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=1152 7=128,"
				"Split            splitexcalibur_98        1 2 /model.80/m_kpt.0/m_kpt.0.2/conv/Conv_output_0 /model.80/m_kpt.0/m_kpt.0.2/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.2/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.0/m_kpt.0.2/act/Sigmoid 1 1 /model.80/m_kpt.0/m_kpt.0.2/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.0/m_kpt.0.2/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.0/m_kpt.0.2/act/Mul 2 1 /model.80/m_kpt.0/m_kpt.0.2/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.2/act/Sigmoid_output_0 /model.80/m_kpt.0/m_kpt.0.2/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.0/m_kpt.0.3/conv/Conv 1 1 /model.80/m_kpt.0/m_kpt.0.2/act/Mul_output_0 /model.80/m_kpt.0/m_kpt.0.3/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=16384,"
				"Split            splitexcalibur_99        1 2 /model.80/m_kpt.0/m_kpt.0.3/conv/Conv_output_0 /model.80/m_kpt.0/m_kpt.0.3/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.3/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.0/m_kpt.0.3/act/Sigmoid 1 1 /model.80/m_kpt.0/m_kpt.0.3/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.0/m_kpt.0.3/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.0/m_kpt.0.3/act/Mul 2 1 /model.80/m_kpt.0/m_kpt.0.3/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.3/act/Sigmoid_output_0 /model.80/m_kpt.0/m_kpt.0.3/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.0/m_kpt.0.4/conv/Conv 1 1 /model.80/m_kpt.0/m_kpt.0.3/act/Mul_output_0 /model.80/m_kpt.0/m_kpt.0.4/conv/Conv_output_0 0=128 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=1152 7=128,"
				"Split            splitexcalibur_100       1 2 /model.80/m_kpt.0/m_kpt.0.4/conv/Conv_output_0 /model.80/m_kpt.0/m_kpt.0.4/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.4/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.0/m_kpt.0.4/act/Sigmoid 1 1 /model.80/m_kpt.0/m_kpt.0.4/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.0/m_kpt.0.4/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.0/m_kpt.0.4/act/Mul 2 1 /model.80/m_kpt.0/m_kpt.0.4/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.4/act/Sigmoid_output_0 /model.80/m_kpt.0/m_kpt.0.4/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.0/m_kpt.0.5/conv/Conv 1 1 /model.80/m_kpt.0/m_kpt.0.4/act/Mul_output_0 /model.80/m_kpt.0/m_kpt.0.5/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=16384,"
				"Split            splitexcalibur_101       1 2 /model.80/m_kpt.0/m_kpt.0.5/conv/Conv_output_0 /model.80/m_kpt.0/m_kpt.0.5/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.5/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.0/m_kpt.0.5/act/Sigmoid 1 1 /model.80/m_kpt.0/m_kpt.0.5/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.0/m_kpt.0.5/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.0/m_kpt.0.5/act/Mul 2 1 /model.80/m_kpt.0/m_kpt.0.5/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.5/act/Sigmoid_output_0 /model.80/m_kpt.0/m_kpt.0.5/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.0/m_kpt.0.6/conv/Conv 1 1 /model.80/m_kpt.0/m_kpt.0.5/act/Mul_output_0 /model.80/m_kpt.0/m_kpt.0.6/conv/Conv_output_0 0=128 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=1152 7=128,"
				"Split            splitexcalibur_102       1 2 /model.80/m_kpt.0/m_kpt.0.6/conv/Conv_output_0 /model.80/m_kpt.0/m_kpt.0.6/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.6/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.0/m_kpt.0.6/act/Sigmoid 1 1 /model.80/m_kpt.0/m_kpt.0.6/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.0/m_kpt.0.6/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.0/m_kpt.0.6/act/Mul 2 1 /model.80/m_kpt.0/m_kpt.0.6/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.6/act/Sigmoid_output_0 /model.80/m_kpt.0/m_kpt.0.6/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.0/m_kpt.0.7/conv/Conv 1 1 /model.80/m_kpt.0/m_kpt.0.6/act/Mul_output_0 /model.80/m_kpt.0/m_kpt.0.7/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=16384,"
				"Split            splitexcalibur_103       1 2 /model.80/m_kpt.0/m_kpt.0.7/conv/Conv_output_0 /model.80/m_kpt.0/m_kpt.0.7/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.7/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.0/m_kpt.0.7/act/Sigmoid 1 1 /model.80/m_kpt.0/m_kpt.0.7/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.0/m_kpt.0.7/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.0/m_kpt.0.7/act/Mul 2 1 /model.80/m_kpt.0/m_kpt.0.7/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.7/act/Sigmoid_output_0 /model.80/m_kpt.0/m_kpt.0.7/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.0/m_kpt.0.8/conv/Conv 1 1 /model.80/m_kpt.0/m_kpt.0.7/act/Mul_output_0 /model.80/m_kpt.0/m_kpt.0.8/conv/Conv_output_0 0=128 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=1152 7=128,"
				"Split            splitexcalibur_104       1 2 /model.80/m_kpt.0/m_kpt.0.8/conv/Conv_output_0 /model.80/m_kpt.0/m_kpt.0.8/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.8/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.0/m_kpt.0.8/act/Sigmoid 1 1 /model.80/m_kpt.0/m_kpt.0.8/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.0/m_kpt.0.8/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.0/m_kpt.0.8/act/Mul 2 1 /model.80/m_kpt.0/m_kpt.0.8/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.8/act/Sigmoid_output_0 /model.80/m_kpt.0/m_kpt.0.8/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.0/m_kpt.0.9/conv/Conv 1 1 /model.80/m_kpt.0/m_kpt.0.8/act/Mul_output_0 /model.80/m_kpt.0/m_kpt.0.9/conv/Conv_output_0 0=128 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=16384,"
				"Split            splitexcalibur_105       1 2 /model.80/m_kpt.0/m_kpt.0.9/conv/Conv_output_0 /model.80/m_kpt.0/m_kpt.0.9/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.9/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.0/m_kpt.0.9/act/Sigmoid 1 1 /model.80/m_kpt.0/m_kpt.0.9/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.0/m_kpt.0.9/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.0/m_kpt.0.9/act/Mul 2 1 /model.80/m_kpt.0/m_kpt.0.9/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.9/act/Sigmoid_output_0 /model.80/m_kpt.0/m_kpt.0.9/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.0/m_kpt.0.10/conv/Conv 1 1 /model.80/m_kpt.0/m_kpt.0.9/act/Mul_output_0 /model.80/m_kpt.0/m_kpt.0.10/conv/Conv_output_0 0=128 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=1152 7=128,"
				"Split            splitexcalibur_106       1 2 /model.80/m_kpt.0/m_kpt.0.10/conv/Conv_output_0 /model.80/m_kpt.0/m_kpt.0.10/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.10/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.0/m_kpt.0.10/act/Sigmoid 1 1 /model.80/m_kpt.0/m_kpt.0.10/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.0/m_kpt.0.10/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.0/m_kpt.0.10/act/Mul 2 1 /model.80/m_kpt.0/m_kpt.0.10/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.0/m_kpt.0.10/act/Sigmoid_output_0 /model.80/m_kpt.0/m_kpt.0.10/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.0/m_kpt.0.11/Conv 1 1 /model.80/m_kpt.0/m_kpt.0.10/act/Mul_output_0 /model.80/m_kpt.0/m_kpt.0.11/Conv_output_0 0=45 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=5760,"
				"Concat           /model.80/Concat         2 1 /model.80/im.0/Mul_output_0 /model.80/m_kpt.0/m_kpt.0.11/Conv_output_0 /model.80/Concat_output_0 0=-1,"
				"Reshape          /model.80/Reshape        1 1 /model.80/Concat_output_0 /model.80/Reshape_output_0 0=6400 1=23 2=3,"
				"Transpose        /model.80/Transpose      1 1 /model.80/Reshape_output_0 475 0=1,0,2,"
				"BinaryOp         /model.80/ia.1/Add       2 1 /model.80/ia.1/Expand_output_0 /model.78/act/Mul_output_0_splitexcalibur_1 /model.80/ia.1/Add_output_0 0=0,"
				"Convolution      /model.80/m.1/Conv       1 1 /model.80/ia.1/Add_output_0 /model.80/m.1/Conv_output_0 0=24 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=6144,"
				"BinaryOp         /model.80/im.1/Mul       2 1 /model.80/im.1/Expand_output_0 /model.80/m.1/Conv_output_0 /model.80/im.1/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.1/m_kpt.1.0/conv/Conv 1 1 /model.78/act/Mul_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.0/conv/Conv_output_0 0=256 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=2304 7=256,"
				"Split            splitexcalibur_107       1 2 /model.80/m_kpt.1/m_kpt.1.0/conv/Conv_output_0 /model.80/m_kpt.1/m_kpt.1.0/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.0/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.1/m_kpt.1.0/act/Sigmoid 1 1 /model.80/m_kpt.1/m_kpt.1.0/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.1/m_kpt.1.0/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.1/m_kpt.1.0/act/Mul 2 1 /model.80/m_kpt.1/m_kpt.1.0/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.0/act/Sigmoid_output_0 /model.80/m_kpt.1/m_kpt.1.0/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.1/m_kpt.1.1/conv/Conv 1 1 /model.80/m_kpt.1/m_kpt.1.0/act/Mul_output_0 /model.80/m_kpt.1/m_kpt.1.1/conv/Conv_output_0 0=256 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=65536,"
				"Split            splitexcalibur_108       1 2 /model.80/m_kpt.1/m_kpt.1.1/conv/Conv_output_0 /model.80/m_kpt.1/m_kpt.1.1/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.1/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.1/m_kpt.1.1/act/Sigmoid 1 1 /model.80/m_kpt.1/m_kpt.1.1/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.1/m_kpt.1.1/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.1/m_kpt.1.1/act/Mul 2 1 /model.80/m_kpt.1/m_kpt.1.1/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.1/act/Sigmoid_output_0 /model.80/m_kpt.1/m_kpt.1.1/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.1/m_kpt.1.2/conv/Conv 1 1 /model.80/m_kpt.1/m_kpt.1.1/act/Mul_output_0 /model.80/m_kpt.1/m_kpt.1.2/conv/Conv_output_0 0=256 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=2304 7=256,"
				"Split            splitexcalibur_109       1 2 /model.80/m_kpt.1/m_kpt.1.2/conv/Conv_output_0 /model.80/m_kpt.1/m_kpt.1.2/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.2/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.1/m_kpt.1.2/act/Sigmoid 1 1 /model.80/m_kpt.1/m_kpt.1.2/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.1/m_kpt.1.2/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.1/m_kpt.1.2/act/Mul 2 1 /model.80/m_kpt.1/m_kpt.1.2/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.2/act/Sigmoid_output_0 /model.80/m_kpt.1/m_kpt.1.2/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.1/m_kpt.1.3/conv/Conv 1 1 /model.80/m_kpt.1/m_kpt.1.2/act/Mul_output_0 /model.80/m_kpt.1/m_kpt.1.3/conv/Conv_output_0 0=256 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=65536,"
				"Split            splitexcalibur_110       1 2 /model.80/m_kpt.1/m_kpt.1.3/conv/Conv_output_0 /model.80/m_kpt.1/m_kpt.1.3/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.3/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.1/m_kpt.1.3/act/Sigmoid 1 1 /model.80/m_kpt.1/m_kpt.1.3/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.1/m_kpt.1.3/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.1/m_kpt.1.3/act/Mul 2 1 /model.80/m_kpt.1/m_kpt.1.3/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.3/act/Sigmoid_output_0 /model.80/m_kpt.1/m_kpt.1.3/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.1/m_kpt.1.4/conv/Conv 1 1 /model.80/m_kpt.1/m_kpt.1.3/act/Mul_output_0 /model.80/m_kpt.1/m_kpt.1.4/conv/Conv_output_0 0=256 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=2304 7=256,"
				"Split            splitexcalibur_111       1 2 /model.80/m_kpt.1/m_kpt.1.4/conv/Conv_output_0 /model.80/m_kpt.1/m_kpt.1.4/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.4/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.1/m_kpt.1.4/act/Sigmoid 1 1 /model.80/m_kpt.1/m_kpt.1.4/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.1/m_kpt.1.4/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.1/m_kpt.1.4/act/Mul 2 1 /model.80/m_kpt.1/m_kpt.1.4/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.4/act/Sigmoid_output_0 /model.80/m_kpt.1/m_kpt.1.4/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.1/m_kpt.1.5/conv/Conv 1 1 /model.80/m_kpt.1/m_kpt.1.4/act/Mul_output_0 /model.80/m_kpt.1/m_kpt.1.5/conv/Conv_output_0 0=256 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=65536,"
				"Split            splitexcalibur_112       1 2 /model.80/m_kpt.1/m_kpt.1.5/conv/Conv_output_0 /model.80/m_kpt.1/m_kpt.1.5/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.5/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.1/m_kpt.1.5/act/Sigmoid 1 1 /model.80/m_kpt.1/m_kpt.1.5/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.1/m_kpt.1.5/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.1/m_kpt.1.5/act/Mul 2 1 /model.80/m_kpt.1/m_kpt.1.5/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.5/act/Sigmoid_output_0 /model.80/m_kpt.1/m_kpt.1.5/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.1/m_kpt.1.6/conv/Conv 1 1 /model.80/m_kpt.1/m_kpt.1.5/act/Mul_output_0 /model.80/m_kpt.1/m_kpt.1.6/conv/Conv_output_0 0=256 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=2304 7=256,"
				"Split            splitexcalibur_113       1 2 /model.80/m_kpt.1/m_kpt.1.6/conv/Conv_output_0 /model.80/m_kpt.1/m_kpt.1.6/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.6/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.1/m_kpt.1.6/act/Sigmoid 1 1 /model.80/m_kpt.1/m_kpt.1.6/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.1/m_kpt.1.6/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.1/m_kpt.1.6/act/Mul 2 1 /model.80/m_kpt.1/m_kpt.1.6/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.6/act/Sigmoid_output_0 /model.80/m_kpt.1/m_kpt.1.6/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.1/m_kpt.1.7/conv/Conv 1 1 /model.80/m_kpt.1/m_kpt.1.6/act/Mul_output_0 /model.80/m_kpt.1/m_kpt.1.7/conv/Conv_output_0 0=256 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=65536,"
				"Split            splitexcalibur_114       1 2 /model.80/m_kpt.1/m_kpt.1.7/conv/Conv_output_0 /model.80/m_kpt.1/m_kpt.1.7/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.7/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.1/m_kpt.1.7/act/Sigmoid 1 1 /model.80/m_kpt.1/m_kpt.1.7/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.1/m_kpt.1.7/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.1/m_kpt.1.7/act/Mul 2 1 /model.80/m_kpt.1/m_kpt.1.7/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.7/act/Sigmoid_output_0 /model.80/m_kpt.1/m_kpt.1.7/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.1/m_kpt.1.8/conv/Conv 1 1 /model.80/m_kpt.1/m_kpt.1.7/act/Mul_output_0 /model.80/m_kpt.1/m_kpt.1.8/conv/Conv_output_0 0=256 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=2304 7=256,"
				"Split            splitexcalibur_115       1 2 /model.80/m_kpt.1/m_kpt.1.8/conv/Conv_output_0 /model.80/m_kpt.1/m_kpt.1.8/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.8/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.1/m_kpt.1.8/act/Sigmoid 1 1 /model.80/m_kpt.1/m_kpt.1.8/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.1/m_kpt.1.8/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.1/m_kpt.1.8/act/Mul 2 1 /model.80/m_kpt.1/m_kpt.1.8/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.8/act/Sigmoid_output_0 /model.80/m_kpt.1/m_kpt.1.8/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.1/m_kpt.1.9/conv/Conv 1 1 /model.80/m_kpt.1/m_kpt.1.8/act/Mul_output_0 /model.80/m_kpt.1/m_kpt.1.9/conv/Conv_output_0 0=256 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=65536,"
				"Split            splitexcalibur_116       1 2 /model.80/m_kpt.1/m_kpt.1.9/conv/Conv_output_0 /model.80/m_kpt.1/m_kpt.1.9/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.9/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.1/m_kpt.1.9/act/Sigmoid 1 1 /model.80/m_kpt.1/m_kpt.1.9/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.1/m_kpt.1.9/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.1/m_kpt.1.9/act/Mul 2 1 /model.80/m_kpt.1/m_kpt.1.9/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.9/act/Sigmoid_output_0 /model.80/m_kpt.1/m_kpt.1.9/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.1/m_kpt.1.10/conv/Conv 1 1 /model.80/m_kpt.1/m_kpt.1.9/act/Mul_output_0 /model.80/m_kpt.1/m_kpt.1.10/conv/Conv_output_0 0=256 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=2304 7=256,"
				"Split            splitexcalibur_117       1 2 /model.80/m_kpt.1/m_kpt.1.10/conv/Conv_output_0 /model.80/m_kpt.1/m_kpt.1.10/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.10/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.1/m_kpt.1.10/act/Sigmoid 1 1 /model.80/m_kpt.1/m_kpt.1.10/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.1/m_kpt.1.10/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.1/m_kpt.1.10/act/Mul 2 1 /model.80/m_kpt.1/m_kpt.1.10/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.1/m_kpt.1.10/act/Sigmoid_output_0 /model.80/m_kpt.1/m_kpt.1.10/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.1/m_kpt.1.11/Conv 1 1 /model.80/m_kpt.1/m_kpt.1.10/act/Mul_output_0 /model.80/m_kpt.1/m_kpt.1.11/Conv_output_0 0=45 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=11520,"
				"Concat           /model.80/Concat_1       2 1 /model.80/im.1/Mul_output_0 /model.80/m_kpt.1/m_kpt.1.11/Conv_output_0 /model.80/Concat_1_output_0 0=-1,"
				"Reshape          /model.80/Reshape_1      1 1 /model.80/Concat_1_output_0 /model.80/Reshape_1_output_0 0=1600 1=23 2=3,"
				"Transpose        /model.80/Transpose_1    1 1 /model.80/Reshape_1_output_0 528 0=1,0,2,"
				"BinaryOp         /model.80/ia.2/Add       2 1 /model.80/ia.2/Expand_output_0 /model.79/act/Mul_output_0_splitexcalibur_1 /model.80/ia.2/Add_output_0 0=0,"
				"Convolution      /model.80/m.2/Conv       1 1 /model.80/ia.2/Add_output_0 /model.80/m.2/Conv_output_0 0=24 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=12288,"
				"BinaryOp         /model.80/im.2/Mul       2 1 /model.80/im.2/Expand_output_0 /model.80/m.2/Conv_output_0 /model.80/im.2/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.2/m_kpt.2.0/conv/Conv 1 1 /model.79/act/Mul_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.0/conv/Conv_output_0 0=512 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=4608 7=512,"
				"Split            splitexcalibur_118       1 2 /model.80/m_kpt.2/m_kpt.2.0/conv/Conv_output_0 /model.80/m_kpt.2/m_kpt.2.0/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.0/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.2/m_kpt.2.0/act/Sigmoid 1 1 /model.80/m_kpt.2/m_kpt.2.0/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.2/m_kpt.2.0/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.2/m_kpt.2.0/act/Mul 2 1 /model.80/m_kpt.2/m_kpt.2.0/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.0/act/Sigmoid_output_0 /model.80/m_kpt.2/m_kpt.2.0/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.2/m_kpt.2.1/conv/Conv 1 1 /model.80/m_kpt.2/m_kpt.2.0/act/Mul_output_0 /model.80/m_kpt.2/m_kpt.2.1/conv/Conv_output_0 0=512 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=262144,"
				"Split            splitexcalibur_119       1 2 /model.80/m_kpt.2/m_kpt.2.1/conv/Conv_output_0 /model.80/m_kpt.2/m_kpt.2.1/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.1/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.2/m_kpt.2.1/act/Sigmoid 1 1 /model.80/m_kpt.2/m_kpt.2.1/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.2/m_kpt.2.1/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.2/m_kpt.2.1/act/Mul 2 1 /model.80/m_kpt.2/m_kpt.2.1/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.1/act/Sigmoid_output_0 /model.80/m_kpt.2/m_kpt.2.1/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.2/m_kpt.2.2/conv/Conv 1 1 /model.80/m_kpt.2/m_kpt.2.1/act/Mul_output_0 /model.80/m_kpt.2/m_kpt.2.2/conv/Conv_output_0 0=512 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=4608 7=512,"
				"Split            splitexcalibur_120       1 2 /model.80/m_kpt.2/m_kpt.2.2/conv/Conv_output_0 /model.80/m_kpt.2/m_kpt.2.2/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.2/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.2/m_kpt.2.2/act/Sigmoid 1 1 /model.80/m_kpt.2/m_kpt.2.2/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.2/m_kpt.2.2/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.2/m_kpt.2.2/act/Mul 2 1 /model.80/m_kpt.2/m_kpt.2.2/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.2/act/Sigmoid_output_0 /model.80/m_kpt.2/m_kpt.2.2/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.2/m_kpt.2.3/conv/Conv 1 1 /model.80/m_kpt.2/m_kpt.2.2/act/Mul_output_0 /model.80/m_kpt.2/m_kpt.2.3/conv/Conv_output_0 0=512 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=262144,"
				"Split            splitexcalibur_121       1 2 /model.80/m_kpt.2/m_kpt.2.3/conv/Conv_output_0 /model.80/m_kpt.2/m_kpt.2.3/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.3/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.2/m_kpt.2.3/act/Sigmoid 1 1 /model.80/m_kpt.2/m_kpt.2.3/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.2/m_kpt.2.3/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.2/m_kpt.2.3/act/Mul 2 1 /model.80/m_kpt.2/m_kpt.2.3/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.3/act/Sigmoid_output_0 /model.80/m_kpt.2/m_kpt.2.3/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.2/m_kpt.2.4/conv/Conv 1 1 /model.80/m_kpt.2/m_kpt.2.3/act/Mul_output_0 /model.80/m_kpt.2/m_kpt.2.4/conv/Conv_output_0 0=512 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=4608 7=512,"
				"Split            splitexcalibur_122       1 2 /model.80/m_kpt.2/m_kpt.2.4/conv/Conv_output_0 /model.80/m_kpt.2/m_kpt.2.4/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.4/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.2/m_kpt.2.4/act/Sigmoid 1 1 /model.80/m_kpt.2/m_kpt.2.4/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.2/m_kpt.2.4/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.2/m_kpt.2.4/act/Mul 2 1 /model.80/m_kpt.2/m_kpt.2.4/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.4/act/Sigmoid_output_0 /model.80/m_kpt.2/m_kpt.2.4/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.2/m_kpt.2.5/conv/Conv 1 1 /model.80/m_kpt.2/m_kpt.2.4/act/Mul_output_0 /model.80/m_kpt.2/m_kpt.2.5/conv/Conv_output_0 0=512 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=262144,"
				"Split            splitexcalibur_123       1 2 /model.80/m_kpt.2/m_kpt.2.5/conv/Conv_output_0 /model.80/m_kpt.2/m_kpt.2.5/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.5/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.2/m_kpt.2.5/act/Sigmoid 1 1 /model.80/m_kpt.2/m_kpt.2.5/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.2/m_kpt.2.5/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.2/m_kpt.2.5/act/Mul 2 1 /model.80/m_kpt.2/m_kpt.2.5/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.5/act/Sigmoid_output_0 /model.80/m_kpt.2/m_kpt.2.5/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.2/m_kpt.2.6/conv/Conv 1 1 /model.80/m_kpt.2/m_kpt.2.5/act/Mul_output_0 /model.80/m_kpt.2/m_kpt.2.6/conv/Conv_output_0 0=512 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=4608 7=512,"
				"Split            splitexcalibur_124       1 2 /model.80/m_kpt.2/m_kpt.2.6/conv/Conv_output_0 /model.80/m_kpt.2/m_kpt.2.6/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.6/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.2/m_kpt.2.6/act/Sigmoid 1 1 /model.80/m_kpt.2/m_kpt.2.6/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.2/m_kpt.2.6/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.2/m_kpt.2.6/act/Mul 2 1 /model.80/m_kpt.2/m_kpt.2.6/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.6/act/Sigmoid_output_0 /model.80/m_kpt.2/m_kpt.2.6/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.2/m_kpt.2.7/conv/Conv 1 1 /model.80/m_kpt.2/m_kpt.2.6/act/Mul_output_0 /model.80/m_kpt.2/m_kpt.2.7/conv/Conv_output_0 0=512 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=262144,"
				"Split            splitexcalibur_125       1 2 /model.80/m_kpt.2/m_kpt.2.7/conv/Conv_output_0 /model.80/m_kpt.2/m_kpt.2.7/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.7/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.2/m_kpt.2.7/act/Sigmoid 1 1 /model.80/m_kpt.2/m_kpt.2.7/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.2/m_kpt.2.7/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.2/m_kpt.2.7/act/Mul 2 1 /model.80/m_kpt.2/m_kpt.2.7/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.7/act/Sigmoid_output_0 /model.80/m_kpt.2/m_kpt.2.7/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.2/m_kpt.2.8/conv/Conv 1 1 /model.80/m_kpt.2/m_kpt.2.7/act/Mul_output_0 /model.80/m_kpt.2/m_kpt.2.8/conv/Conv_output_0 0=512 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=4608 7=512,"
				"Split            splitexcalibur_126       1 2 /model.80/m_kpt.2/m_kpt.2.8/conv/Conv_output_0 /model.80/m_kpt.2/m_kpt.2.8/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.8/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.2/m_kpt.2.8/act/Sigmoid 1 1 /model.80/m_kpt.2/m_kpt.2.8/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.2/m_kpt.2.8/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.2/m_kpt.2.8/act/Mul 2 1 /model.80/m_kpt.2/m_kpt.2.8/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.8/act/Sigmoid_output_0 /model.80/m_kpt.2/m_kpt.2.8/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.2/m_kpt.2.9/conv/Conv 1 1 /model.80/m_kpt.2/m_kpt.2.8/act/Mul_output_0 /model.80/m_kpt.2/m_kpt.2.9/conv/Conv_output_0 0=512 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=262144,"
				"Split            splitexcalibur_127       1 2 /model.80/m_kpt.2/m_kpt.2.9/conv/Conv_output_0 /model.80/m_kpt.2/m_kpt.2.9/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.9/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.2/m_kpt.2.9/act/Sigmoid 1 1 /model.80/m_kpt.2/m_kpt.2.9/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.2/m_kpt.2.9/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.2/m_kpt.2.9/act/Mul 2 1 /model.80/m_kpt.2/m_kpt.2.9/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.9/act/Sigmoid_output_0 /model.80/m_kpt.2/m_kpt.2.9/act/Mul_output_0 0=2,"
				"ConvolutionDepthWise /model.80/m_kpt.2/m_kpt.2.10/conv/Conv 1 1 /model.80/m_kpt.2/m_kpt.2.9/act/Mul_output_0 /model.80/m_kpt.2/m_kpt.2.10/conv/Conv_output_0 0=512 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=4608 7=512,"
				"Split            splitexcalibur_128       1 2 /model.80/m_kpt.2/m_kpt.2.10/conv/Conv_output_0 /model.80/m_kpt.2/m_kpt.2.10/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.10/conv/Conv_output_0_splitexcalibur_1,"
				"Sigmoid          /model.80/m_kpt.2/m_kpt.2.10/act/Sigmoid 1 1 /model.80/m_kpt.2/m_kpt.2.10/conv/Conv_output_0_splitexcalibur_1 /model.80/m_kpt.2/m_kpt.2.10/act/Sigmoid_output_0,"
				"BinaryOp         /model.80/m_kpt.2/m_kpt.2.10/act/Mul 2 1 /model.80/m_kpt.2/m_kpt.2.10/conv/Conv_output_0_splitexcalibur_0 /model.80/m_kpt.2/m_kpt.2.10/act/Sigmoid_output_0 /model.80/m_kpt.2/m_kpt.2.10/act/Mul_output_0 0=2,"
				"Convolution      /model.80/m_kpt.2/m_kpt.2.11/Conv 1 1 /model.80/m_kpt.2/m_kpt.2.10/act/Mul_output_0 /model.80/m_kpt.2/m_kpt.2.11/Conv_output_0 0=45 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=23040,"
				"Concat           /model.80/Concat_2       2 1 /model.80/im.2/Mul_output_0 /model.80/m_kpt.2/m_kpt.2.11/Conv_output_0 /model.80/Concat_2_output_0 0=-1,"
				"Reshape          /model.80/Reshape_2      1 1 /model.80/Concat_2_output_0 /model.80/Reshape_2_output_0 0=400 1=23 2=3,"
				"Transpose        /model.80/Transpose_2    1 1 /model.80/Reshape_2_output_0 581 0=1,0,2,"
            };
            inline static const std::vector<std::string> posture{
                    "glsv1",
                    "533 683",
                    "Input            images                   0 1 images 0=640 1=640 2=3 3=0,0,0 4=0.0039215",
                    "Yolov5focus      yolov5focus              1 1 images 281",
                    "Convolution      Conv_45                  1 1 281 282 0=48 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=5184",
                    "Split            splitexcalibur_0         1 2 282 282_splitexcalibur_0 282_splitexcalibur_1",
                    "Sigmoid          Sigmoid_46               1 1 282_splitexcalibur_1 283",
                    "BinaryOp         Mul_47                   2 1 282_splitexcalibur_0 283 284 0=2",
                    "Convolution      Conv_48                  1 1 284 285 0=96 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=41472",
                    "Split            splitexcalibur_1         1 2 285 285_splitexcalibur_0 285_splitexcalibur_1",
                    "Sigmoid          Sigmoid_49               1 1 285_splitexcalibur_1 286",
                    "BinaryOp         Mul_50                   2 1 285_splitexcalibur_0 286 287 0=2",
                    "Split            splitexcalibur_2         1 2 287 287_splitexcalibur_0 287_splitexcalibur_1",
                    "Convolution      Conv_51                  1 1 287_splitexcalibur_1 288 0=48 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=4608",
                    "Split            splitexcalibur_3         1 2 288 288_splitexcalibur_0 288_splitexcalibur_1",
                    "Sigmoid          Sigmoid_52               1 1 288_splitexcalibur_1 289",
                    "BinaryOp         Mul_53                   2 1 288_splitexcalibur_0 289 290 0=2",
                    "Split            splitexcalibur_4         1 2 290 290_splitexcalibur_0 290_splitexcalibur_1",
                    "Convolution      Conv_54                  1 1 290_splitexcalibur_1 291 0=48 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=2304",
                    "Split            splitexcalibur_5         1 2 291 291_splitexcalibur_0 291_splitexcalibur_1",
                    "Sigmoid          Sigmoid_55               1 1 291_splitexcalibur_1 292",
                    "BinaryOp         Mul_56                   2 1 291_splitexcalibur_0 292 293 0=2",
                    "Convolution      Conv_57                  1 1 293 294 0=48 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=20736",
                    "Split            splitexcalibur_6         1 2 294 294_splitexcalibur_0 294_splitexcalibur_1",
                    "Sigmoid          Sigmoid_58               1 1 294_splitexcalibur_1 295",
                    "BinaryOp         Mul_59                   2 1 294_splitexcalibur_0 295 296 0=2",
                    "BinaryOp         Add_60                   2 1 290_splitexcalibur_0 296 297 0=0",
                    "Split            splitexcalibur_7         1 2 297 297_splitexcalibur_0 297_splitexcalibur_1",
                    "Convolution      Conv_61                  1 1 297_splitexcalibur_1 298 0=48 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=2304",
                    "Split            splitexcalibur_8         1 2 298 298_splitexcalibur_0 298_splitexcalibur_1",
                    "Sigmoid          Sigmoid_62               1 1 298_splitexcalibur_1 299",
                    "BinaryOp         Mul_63                   2 1 298_splitexcalibur_0 299 300 0=2",
                    "Convolution      Conv_64                  1 1 300 301 0=48 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=20736",
                    "Split            splitexcalibur_9         1 2 301 301_splitexcalibur_0 301_splitexcalibur_1",
                    "Sigmoid          Sigmoid_65               1 1 301_splitexcalibur_1 302",
                    "BinaryOp         Mul_66                   2 1 301_splitexcalibur_0 302 303 0=2",
                    "BinaryOp         Add_67                   2 1 297_splitexcalibur_0 303 304 0=0",
                    "Convolution      Conv_68                  1 1 287_splitexcalibur_0 305 0=48 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=4608",
                    "Split            splitexcalibur_10        1 2 305 305_splitexcalibur_0 305_splitexcalibur_1",
                    "Sigmoid          Sigmoid_69               1 1 305_splitexcalibur_1 306",
                    "BinaryOp         Mul_70                   2 1 305_splitexcalibur_0 306 307 0=2",
                    "Concat           Concat_71                2 1 304 307 308 0=-1",
                    "Convolution      Conv_72                  1 1 308 309 0=96 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=9216",
                    "Split            splitexcalibur_11        1 2 309 309_splitexcalibur_0 309_splitexcalibur_1",
                    "Sigmoid          Sigmoid_73               1 1 309_splitexcalibur_1 310",
                    "BinaryOp         Mul_74                   2 1 309_splitexcalibur_0 310 311 0=2",
                    "Convolution      Conv_75                  1 1 311 312 0=192 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=165888",
                    "Split            splitexcalibur_12        1 2 312 312_splitexcalibur_0 312_splitexcalibur_1",
                    "Sigmoid          Sigmoid_76               1 1 312_splitexcalibur_1 313",
                    "BinaryOp         Mul_77                   2 1 312_splitexcalibur_0 313 314 0=2",
                    "Split            splitexcalibur_13        1 2 314 314_splitexcalibur_0 314_splitexcalibur_1",
                    "Convolution      Conv_78                  1 1 314_splitexcalibur_1 315 0=96 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=18432",
                    "Split            splitexcalibur_14        1 2 315 315_splitexcalibur_0 315_splitexcalibur_1",
                    "Sigmoid          Sigmoid_79               1 1 315_splitexcalibur_1 316",
                    "BinaryOp         Mul_80                   2 1 315_splitexcalibur_0 316 317 0=2",
                    "Split            splitexcalibur_15        1 2 317 317_splitexcalibur_0 317_splitexcalibur_1",
                    "Convolution      Conv_81                  1 1 317_splitexcalibur_1 318 0=96 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=9216",
                    "Split            splitexcalibur_16        1 2 318 318_splitexcalibur_0 318_splitexcalibur_1",
                    "Sigmoid          Sigmoid_82               1 1 318_splitexcalibur_1 319",
                    "BinaryOp         Mul_83                   2 1 318_splitexcalibur_0 319 320 0=2",
                    "Convolution      Conv_84                  1 1 320 321 0=96 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=82944",
                    "Split            splitexcalibur_17        1 2 321 321_splitexcalibur_0 321_splitexcalibur_1",
                    "Sigmoid          Sigmoid_85               1 1 321_splitexcalibur_1 322",
                    "BinaryOp         Mul_86                   2 1 321_splitexcalibur_0 322 323 0=2",
                    "BinaryOp         Add_87                   2 1 317_splitexcalibur_0 323 324 0=0",
                    "Split            splitexcalibur_18        1 2 324 324_splitexcalibur_0 324_splitexcalibur_1",
                    "Convolution      Conv_88                  1 1 324_splitexcalibur_1 325 0=96 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=9216",
                    "Split            splitexcalibur_19        1 2 325 325_splitexcalibur_0 325_splitexcalibur_1",
                    "Sigmoid          Sigmoid_89               1 1 325_splitexcalibur_1 326",
                    "BinaryOp         Mul_90                   2 1 325_splitexcalibur_0 326 327 0=2",
                    "Convolution      Conv_91                  1 1 327 328 0=96 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=82944",
                    "Split            splitexcalibur_20        1 2 328 328_splitexcalibur_0 328_splitexcalibur_1",
                    "Sigmoid          Sigmoid_92               1 1 328_splitexcalibur_1 329",
                    "BinaryOp         Mul_93                   2 1 328_splitexcalibur_0 329 330 0=2",
                    "BinaryOp         Add_94                   2 1 324_splitexcalibur_0 330 331 0=0",
                    "Split            splitexcalibur_21        1 2 331 331_splitexcalibur_0 331_splitexcalibur_1",
                    "Convolution      Conv_95                  1 1 331_splitexcalibur_1 332 0=96 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=9216",
                    "Split            splitexcalibur_22        1 2 332 332_splitexcalibur_0 332_splitexcalibur_1",
                    "Sigmoid          Sigmoid_96               1 1 332_splitexcalibur_1 333",
                    "BinaryOp         Mul_97                   2 1 332_splitexcalibur_0 333 334 0=2",
                    "Convolution      Conv_98                  1 1 334 335 0=96 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=82944",
                    "Split            splitexcalibur_23        1 2 335 335_splitexcalibur_0 335_splitexcalibur_1",
                    "Sigmoid          Sigmoid_99               1 1 335_splitexcalibur_1 336",
                    "BinaryOp         Mul_100                  2 1 335_splitexcalibur_0 336 337 0=2",
                    "BinaryOp         Add_101                  2 1 331_splitexcalibur_0 337 338 0=0",
                    "Split            splitexcalibur_24        1 2 338 338_splitexcalibur_0 338_splitexcalibur_1",
                    "Convolution      Conv_102                 1 1 338_splitexcalibur_1 339 0=96 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=9216",
                    "Split            splitexcalibur_25        1 2 339 339_splitexcalibur_0 339_splitexcalibur_1",
                    "Sigmoid          Sigmoid_103              1 1 339_splitexcalibur_1 340",
                    "BinaryOp         Mul_104                  2 1 339_splitexcalibur_0 340 341 0=2",
                    "Convolution      Conv_105                 1 1 341 342 0=96 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=82944",
                    "Split            splitexcalibur_26        1 2 342 342_splitexcalibur_0 342_splitexcalibur_1",
                    "Sigmoid          Sigmoid_106              1 1 342_splitexcalibur_1 343",
                    "BinaryOp         Mul_107                  2 1 342_splitexcalibur_0 343 344 0=2",
                    "BinaryOp         Add_108                  2 1 338_splitexcalibur_0 344 345 0=0",
                    "Split            splitexcalibur_27        1 2 345 345_splitexcalibur_0 345_splitexcalibur_1",
                    "Convolution      Conv_109                 1 1 345_splitexcalibur_1 346 0=96 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=9216",
                    "Split            splitexcalibur_28        1 2 346 346_splitexcalibur_0 346_splitexcalibur_1",
                    "Sigmoid          Sigmoid_110              1 1 346_splitexcalibur_1 347",
                    "BinaryOp         Mul_111                  2 1 346_splitexcalibur_0 347 348 0=2",
                    "Convolution      Conv_112                 1 1 348 349 0=96 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=82944",
                    "Split            splitexcalibur_29        1 2 349 349_splitexcalibur_0 349_splitexcalibur_1",
                    "Sigmoid          Sigmoid_113              1 1 349_splitexcalibur_1 350",
                    "BinaryOp         Mul_114                  2 1 349_splitexcalibur_0 350 351 0=2",
                    "BinaryOp         Add_115                  2 1 345_splitexcalibur_0 351 352 0=0",
                    "Split            splitexcalibur_30        1 2 352 352_splitexcalibur_0 352_splitexcalibur_1",
                    "Convolution      Conv_116                 1 1 352_splitexcalibur_1 353 0=96 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=9216",
                    "Split            splitexcalibur_31        1 2 353 353_splitexcalibur_0 353_splitexcalibur_1",
                    "Sigmoid          Sigmoid_117              1 1 353_splitexcalibur_1 354",
                    "BinaryOp         Mul_118                  2 1 353_splitexcalibur_0 354 355 0=2",
                    "Convolution      Conv_119                 1 1 355 356 0=96 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=82944",
                    "Split            splitexcalibur_32        1 2 356 356_splitexcalibur_0 356_splitexcalibur_1",
                    "Sigmoid          Sigmoid_120              1 1 356_splitexcalibur_1 357",
                    "BinaryOp         Mul_121                  2 1 356_splitexcalibur_0 357 358 0=2",
                    "BinaryOp         Add_122                  2 1 352_splitexcalibur_0 358 359 0=0",
                    "Convolution      Conv_123                 1 1 314_splitexcalibur_0 360 0=96 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=18432",
                    "Split            splitexcalibur_33        1 2 360 360_splitexcalibur_0 360_splitexcalibur_1",
                    "Sigmoid          Sigmoid_124              1 1 360_splitexcalibur_1 361",
                    "BinaryOp         Mul_125                  2 1 360_splitexcalibur_0 361 362 0=2",
                    "Concat           Concat_126               2 1 359 362 363 0=-1",
                    "Convolution      Conv_127                 1 1 363 364 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=36864",
                    "Split            splitexcalibur_34        1 2 364 364_splitexcalibur_0 364_splitexcalibur_1",
                    "Sigmoid          Sigmoid_128              1 1 364_splitexcalibur_1 365",
                    "BinaryOp         Mul_129                  2 1 364_splitexcalibur_0 365 366 0=2",
                    "Split            splitexcalibur_35        1 2 366 366_splitexcalibur_0 366_splitexcalibur_1",
                    "Convolution      Conv_130                 1 1 366_splitexcalibur_1 367 0=384 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=663552",
                    "Split            splitexcalibur_36        1 2 367 367_splitexcalibur_0 367_splitexcalibur_1",
                    "Sigmoid          Sigmoid_131              1 1 367_splitexcalibur_1 368",
                    "BinaryOp         Mul_132                  2 1 367_splitexcalibur_0 368 369 0=2",
                    "Split            splitexcalibur_37        1 2 369 369_splitexcalibur_0 369_splitexcalibur_1",
                    "Convolution      Conv_133                 1 1 369_splitexcalibur_1 370 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=73728",
                    "Split            splitexcalibur_38        1 2 370 370_splitexcalibur_0 370_splitexcalibur_1",
                    "Sigmoid          Sigmoid_134              1 1 370_splitexcalibur_1 371",
                    "BinaryOp         Mul_135                  2 1 370_splitexcalibur_0 371 372 0=2",
                    "Split            splitexcalibur_39        1 2 372 372_splitexcalibur_0 372_splitexcalibur_1",
                    "Convolution      Conv_136                 1 1 372_splitexcalibur_1 373 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=36864",
                    "Split            splitexcalibur_40        1 2 373 373_splitexcalibur_0 373_splitexcalibur_1",
                    "Sigmoid          Sigmoid_137              1 1 373_splitexcalibur_1 374",
                    "BinaryOp         Mul_138                  2 1 373_splitexcalibur_0 374 375 0=2",
                    "Convolution      Conv_139                 1 1 375 376 0=192 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=331776",
                    "Split            splitexcalibur_41        1 2 376 376_splitexcalibur_0 376_splitexcalibur_1",
                    "Sigmoid          Sigmoid_140              1 1 376_splitexcalibur_1 377",
                    "BinaryOp         Mul_141                  2 1 376_splitexcalibur_0 377 378 0=2",
                    "BinaryOp         Add_142                  2 1 372_splitexcalibur_0 378 379 0=0",
                    "Split            splitexcalibur_42        1 2 379 379_splitexcalibur_0 379_splitexcalibur_1",
                    "Convolution      Conv_143                 1 1 379_splitexcalibur_1 380 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=36864",
                    "Split            splitexcalibur_43        1 2 380 380_splitexcalibur_0 380_splitexcalibur_1",
                    "Sigmoid          Sigmoid_144              1 1 380_splitexcalibur_1 381",
                    "BinaryOp         Mul_145                  2 1 380_splitexcalibur_0 381 382 0=2",
                    "Convolution      Conv_146                 1 1 382 383 0=192 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=331776",
                    "Split            splitexcalibur_44        1 2 383 383_splitexcalibur_0 383_splitexcalibur_1",
                    "Sigmoid          Sigmoid_147              1 1 383_splitexcalibur_1 384",
                    "BinaryOp         Mul_148                  2 1 383_splitexcalibur_0 384 385 0=2",
                    "BinaryOp         Add_149                  2 1 379_splitexcalibur_0 385 386 0=0",
                    "Split            splitexcalibur_45        1 2 386 386_splitexcalibur_0 386_splitexcalibur_1",
                    "Convolution      Conv_150                 1 1 386_splitexcalibur_1 387 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=36864",
                    "Split            splitexcalibur_46        1 2 387 387_splitexcalibur_0 387_splitexcalibur_1",
                    "Sigmoid          Sigmoid_151              1 1 387_splitexcalibur_1 388",
                    "BinaryOp         Mul_152                  2 1 387_splitexcalibur_0 388 389 0=2",
                    "Convolution      Conv_153                 1 1 389 390 0=192 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=331776",
                    "Split            splitexcalibur_47        1 2 390 390_splitexcalibur_0 390_splitexcalibur_1",
                    "Sigmoid          Sigmoid_154              1 1 390_splitexcalibur_1 391",
                    "BinaryOp         Mul_155                  2 1 390_splitexcalibur_0 391 392 0=2",
                    "BinaryOp         Add_156                  2 1 386_splitexcalibur_0 392 393 0=0",
                    "Split            splitexcalibur_48        1 2 393 393_splitexcalibur_0 393_splitexcalibur_1",
                    "Convolution      Conv_157                 1 1 393_splitexcalibur_1 394 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=36864",
                    "Split            splitexcalibur_49        1 2 394 394_splitexcalibur_0 394_splitexcalibur_1",
                    "Sigmoid          Sigmoid_158              1 1 394_splitexcalibur_1 395",
                    "BinaryOp         Mul_159                  2 1 394_splitexcalibur_0 395 396 0=2",
                    "Convolution      Conv_160                 1 1 396 397 0=192 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=331776",
                    "Split            splitexcalibur_50        1 2 397 397_splitexcalibur_0 397_splitexcalibur_1",
                    "Sigmoid          Sigmoid_161              1 1 397_splitexcalibur_1 398",
                    "BinaryOp         Mul_162                  2 1 397_splitexcalibur_0 398 399 0=2",
                    "BinaryOp         Add_163                  2 1 393_splitexcalibur_0 399 400 0=0",
                    "Split            splitexcalibur_51        1 2 400 400_splitexcalibur_0 400_splitexcalibur_1",
                    "Convolution      Conv_164                 1 1 400_splitexcalibur_1 401 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=36864",
                    "Split            splitexcalibur_52        1 2 401 401_splitexcalibur_0 401_splitexcalibur_1",
                    "Sigmoid          Sigmoid_165              1 1 401_splitexcalibur_1 402",
                    "BinaryOp         Mul_166                  2 1 401_splitexcalibur_0 402 403 0=2",
                    "Convolution      Conv_167                 1 1 403 404 0=192 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=331776",
                    "Split            splitexcalibur_53        1 2 404 404_splitexcalibur_0 404_splitexcalibur_1",
                    "Sigmoid          Sigmoid_168              1 1 404_splitexcalibur_1 405",
                    "BinaryOp         Mul_169                  2 1 404_splitexcalibur_0 405 406 0=2",
                    "BinaryOp         Add_170                  2 1 400_splitexcalibur_0 406 407 0=0",
                    "Split            splitexcalibur_54        1 2 407 407_splitexcalibur_0 407_splitexcalibur_1",
                    "Convolution      Conv_171                 1 1 407_splitexcalibur_1 408 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=36864",
                    "Split            splitexcalibur_55        1 2 408 408_splitexcalibur_0 408_splitexcalibur_1",
                    "Sigmoid          Sigmoid_172              1 1 408_splitexcalibur_1 409",
                    "BinaryOp         Mul_173                  2 1 408_splitexcalibur_0 409 410 0=2",
                    "Convolution      Conv_174                 1 1 410 411 0=192 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=331776",
                    "Split            splitexcalibur_56        1 2 411 411_splitexcalibur_0 411_splitexcalibur_1",
                    "Sigmoid          Sigmoid_175              1 1 411_splitexcalibur_1 412",
                    "BinaryOp         Mul_176                  2 1 411_splitexcalibur_0 412 413 0=2",
                    "BinaryOp         Add_177                  2 1 407_splitexcalibur_0 413 414 0=0",
                    "Convolution      Conv_178                 1 1 369_splitexcalibur_0 415 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=73728",
                    "Split            splitexcalibur_57        1 2 415 415_splitexcalibur_0 415_splitexcalibur_1",
                    "Sigmoid          Sigmoid_179              1 1 415_splitexcalibur_1 416",
                    "BinaryOp         Mul_180                  2 1 415_splitexcalibur_0 416 417 0=2",
                    "Concat           Concat_181               2 1 414 417 418 0=-1",
                    "Convolution      Conv_182                 1 1 418 419 0=384 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=147456",
                    "Split            splitexcalibur_58        1 2 419 419_splitexcalibur_0 419_splitexcalibur_1",
                    "Sigmoid          Sigmoid_183              1 1 419_splitexcalibur_1 420",
                    "BinaryOp         Mul_184                  2 1 419_splitexcalibur_0 420 421 0=2",
                    "Split            splitexcalibur_59        1 2 421 421_splitexcalibur_0 421_splitexcalibur_1",
                    "Convolution      Conv_185                 1 1 421_splitexcalibur_1 422 0=576 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=1990656",
                    "Split            splitexcalibur_60        1 2 422 422_splitexcalibur_0 422_splitexcalibur_1",
                    "Sigmoid          Sigmoid_186              1 1 422_splitexcalibur_1 423",
                    "BinaryOp         Mul_187                  2 1 422_splitexcalibur_0 423 424 0=2",
                    "Split            splitexcalibur_61        1 2 424 424_splitexcalibur_0 424_splitexcalibur_1",
                    "Convolution      Conv_188                 1 1 424_splitexcalibur_1 425 0=288 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=165888",
                    "Split            splitexcalibur_62        1 2 425 425_splitexcalibur_0 425_splitexcalibur_1",
                    "Sigmoid          Sigmoid_189              1 1 425_splitexcalibur_1 426",
                    "BinaryOp         Mul_190                  2 1 425_splitexcalibur_0 426 427 0=2",
                    "Split            splitexcalibur_63        1 2 427 427_splitexcalibur_0 427_splitexcalibur_1",
                    "Convolution      Conv_191                 1 1 427_splitexcalibur_1 428 0=288 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=82944",
                    "Split            splitexcalibur_64        1 2 428 428_splitexcalibur_0 428_splitexcalibur_1",
                    "Sigmoid          Sigmoid_192              1 1 428_splitexcalibur_1 429",
                    "BinaryOp         Mul_193                  2 1 428_splitexcalibur_0 429 430 0=2",
                    "Convolution      Conv_194                 1 1 430 431 0=288 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=746496",
                    "Split            splitexcalibur_65        1 2 431 431_splitexcalibur_0 431_splitexcalibur_1",
                    "Sigmoid          Sigmoid_195              1 1 431_splitexcalibur_1 432",
                    "BinaryOp         Mul_196                  2 1 431_splitexcalibur_0 432 433 0=2",
                    "BinaryOp         Add_197                  2 1 427_splitexcalibur_0 433 434 0=0",
                    "Split            splitexcalibur_66        1 2 434 434_splitexcalibur_0 434_splitexcalibur_1",
                    "Convolution      Conv_198                 1 1 434_splitexcalibur_1 435 0=288 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=82944",
                    "Split            splitexcalibur_67        1 2 435 435_splitexcalibur_0 435_splitexcalibur_1",
                    "Sigmoid          Sigmoid_199              1 1 435_splitexcalibur_1 436",
                    "BinaryOp         Mul_200                  2 1 435_splitexcalibur_0 436 437 0=2",
                    "Convolution      Conv_201                 1 1 437 438 0=288 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=746496",
                    "Split            splitexcalibur_68        1 2 438 438_splitexcalibur_0 438_splitexcalibur_1",
                    "Sigmoid          Sigmoid_202              1 1 438_splitexcalibur_1 439",
                    "BinaryOp         Mul_203                  2 1 438_splitexcalibur_0 439 440 0=2",
                    "BinaryOp         Add_204                  2 1 434_splitexcalibur_0 440 441 0=0",
                    "Convolution      Conv_205                 1 1 424_splitexcalibur_0 442 0=288 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=165888",
                    "Split            splitexcalibur_69        1 2 442 442_splitexcalibur_0 442_splitexcalibur_1",
                    "Sigmoid          Sigmoid_206              1 1 442_splitexcalibur_1 443",
                    "BinaryOp         Mul_207                  2 1 442_splitexcalibur_0 443 444 0=2",
                    "Concat           Concat_208               2 1 441 444 445 0=-1",
                    "Convolution      Conv_209                 1 1 445 446 0=576 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=331776",
                    "Split            splitexcalibur_70        1 2 446 446_splitexcalibur_0 446_splitexcalibur_1",
                    "Sigmoid          Sigmoid_210              1 1 446_splitexcalibur_1 447",
                    "BinaryOp         Mul_211                  2 1 446_splitexcalibur_0 447 448 0=2",
                    "Split            splitexcalibur_71        1 2 448 448_splitexcalibur_0 448_splitexcalibur_1",
                    "Convolution      Conv_212                 1 1 448_splitexcalibur_1 449 0=768 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=3981312",
                    "Split            splitexcalibur_72        1 2 449 449_splitexcalibur_0 449_splitexcalibur_1",
                    "Sigmoid          Sigmoid_213              1 1 449_splitexcalibur_1 450",
                    "BinaryOp         Mul_214                  2 1 449_splitexcalibur_0 450 451 0=2",
                    "Convolution      Conv_215                 1 1 451 452 0=384 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=294912",
                    "Split            splitexcalibur_73        1 2 452 452_splitexcalibur_0 452_splitexcalibur_1",
                    "Sigmoid          Sigmoid_216              1 1 452_splitexcalibur_1 453",
                    "BinaryOp         Mul_217                  2 1 452_splitexcalibur_0 453 454 0=2",
                    "Split            splitexcalibur_74        1 4 454 454_splitexcalibur_0 454_splitexcalibur_1 454_splitexcalibur_2 454_splitexcalibur_3",
                    "Pooling          MaxPool_218              1 1 454_splitexcalibur_3 455 0=0 1=3 11=3 2=1 12=1 3=1 13=1 14=1 15=1 5=1",
                    "Pooling          MaxPool_219              1 1 454_splitexcalibur_2 456 0=0 1=3 11=3 2=1 12=1 3=1 13=1 14=1 15=1 5=1",
                    "Pooling          MaxPool_220              1 1 456 457 0=0 1=3 11=3 2=1 12=1 3=1 13=1 14=1 15=1 5=1",
                    "Pooling          MaxPool_221              1 1 454_splitexcalibur_1 458 0=0 1=3 11=3 2=1 12=1 3=1 13=1 14=1 15=1 5=1",
                    "Pooling          MaxPool_222              1 1 458 459 0=0 1=3 11=3 2=1 12=1 3=1 13=1 14=1 15=1 5=1",
                    "Pooling          MaxPool_223              1 1 459 460 0=0 1=3 11=3 2=1 12=1 3=1 13=1 14=1 15=1 5=1",
                    "Concat           Concat_224               4 1 454_splitexcalibur_0 455 457 460 461 0=-1",
                    "Convolution      Conv_225                 1 1 461 462 0=768 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=1179648",
                    "Split            splitexcalibur_75        1 2 462 462_splitexcalibur_0 462_splitexcalibur_1",
                    "Sigmoid          Sigmoid_226              1 1 462_splitexcalibur_1 463",
                    "BinaryOp         Mul_227                  2 1 462_splitexcalibur_0 463 464 0=2",
                    "Split            splitexcalibur_76        1 2 464 464_splitexcalibur_0 464_splitexcalibur_1",
                    "Convolution      Conv_228                 1 1 464_splitexcalibur_1 465 0=384 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=294912",
                    "Split            splitexcalibur_77        1 2 465 465_splitexcalibur_0 465_splitexcalibur_1",
                    "Sigmoid          Sigmoid_229              1 1 465_splitexcalibur_1 466",
                    "BinaryOp         Mul_230                  2 1 465_splitexcalibur_0 466 467 0=2",
                    "Convolution      Conv_231                 1 1 467 468 0=384 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=147456",
                    "Split            splitexcalibur_78        1 2 468 468_splitexcalibur_0 468_splitexcalibur_1",
                    "Sigmoid          Sigmoid_232              1 1 468_splitexcalibur_1 469",
                    "BinaryOp         Mul_233                  2 1 468_splitexcalibur_0 469 470 0=2",
                    "Convolution      Conv_234                 1 1 470 471 0=384 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=1327104",
                    "Split            splitexcalibur_79        1 2 471 471_splitexcalibur_0 471_splitexcalibur_1",
                    "Sigmoid          Sigmoid_235              1 1 471_splitexcalibur_1 472",
                    "BinaryOp         Mul_236                  2 1 471_splitexcalibur_0 472 473 0=2",
                    "Convolution      Conv_237                 1 1 473 474 0=384 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=147456",
                    "Split            splitexcalibur_80        1 2 474 474_splitexcalibur_0 474_splitexcalibur_1",
                    "Sigmoid          Sigmoid_238              1 1 474_splitexcalibur_1 475",
                    "BinaryOp         Mul_239                  2 1 474_splitexcalibur_0 475 476 0=2",
                    "Convolution      Conv_240                 1 1 476 477 0=384 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=1327104",
                    "Split            splitexcalibur_81        1 2 477 477_splitexcalibur_0 477_splitexcalibur_1",
                    "Sigmoid          Sigmoid_241              1 1 477_splitexcalibur_1 478",
                    "BinaryOp         Mul_242                  2 1 477_splitexcalibur_0 478 479 0=2",
                    "Convolution      Conv_243                 1 1 464_splitexcalibur_0 480 0=384 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=294912",
                    "Split            splitexcalibur_82        1 2 480 480_splitexcalibur_0 480_splitexcalibur_1",
                    "Sigmoid          Sigmoid_244              1 1 480_splitexcalibur_1 481",
                    "BinaryOp         Mul_245                  2 1 480_splitexcalibur_0 481 482 0=2",
                    "Concat           Concat_246               2 1 479 482 483 0=-1",
                    "Convolution      Conv_247                 1 1 483 484 0=768 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=589824",
                    "Split            splitexcalibur_83        1 2 484 484_splitexcalibur_0 484_splitexcalibur_1",
                    "Sigmoid          Sigmoid_248              1 1 484_splitexcalibur_1 485",
                    "BinaryOp         Mul_249                  2 1 484_splitexcalibur_0 485 486 0=2",
                    "Convolution      Conv_250                 1 1 486 487 0=576 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=442368",
                    "Split            splitexcalibur_84        1 2 487 487_splitexcalibur_0 487_splitexcalibur_1",
                    "Sigmoid          Sigmoid_251              1 1 487_splitexcalibur_1 488",
                    "BinaryOp         Mul_252                  2 1 487_splitexcalibur_0 488 489 0=2",
                    "Split            splitexcalibur_85        1 2 489 489_splitexcalibur_0 489_splitexcalibur_1",
                    "Interp           Resize_254               1 1 489_splitexcalibur_1 494 0=1 1=2.000000e+00 2=2.000000e+00 3=0 4=0 6=0",
                    "Concat           Concat_255               2 1 494 448_splitexcalibur_0 495 0=-1",
                    "Split            splitexcalibur_86        1 2 495 495_splitexcalibur_0 495_splitexcalibur_1",
                    "Convolution      Conv_256                 1 1 495_splitexcalibur_1 496 0=288 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=331776",
                    "Split            splitexcalibur_87        1 2 496 496_splitexcalibur_0 496_splitexcalibur_1",
                    "Sigmoid          Sigmoid_257              1 1 496_splitexcalibur_1 497",
                    "BinaryOp         Mul_258                  2 1 496_splitexcalibur_0 497 498 0=2",
                    "Convolution      Conv_259                 1 1 498 499 0=288 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=82944",
                    "Split            splitexcalibur_88        1 2 499 499_splitexcalibur_0 499_splitexcalibur_1",
                    "Sigmoid          Sigmoid_260              1 1 499_splitexcalibur_1 500",
                    "BinaryOp         Mul_261                  2 1 499_splitexcalibur_0 500 501 0=2",
                    "Convolution      Conv_262                 1 1 501 502 0=288 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=746496",
                    "Split            splitexcalibur_89        1 2 502 502_splitexcalibur_0 502_splitexcalibur_1",
                    "Sigmoid          Sigmoid_263              1 1 502_splitexcalibur_1 503",
                    "BinaryOp         Mul_264                  2 1 502_splitexcalibur_0 503 504 0=2",
                    "Convolution      Conv_265                 1 1 504 505 0=288 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=82944",
                    "Split            splitexcalibur_90        1 2 505 505_splitexcalibur_0 505_splitexcalibur_1",
                    "Sigmoid          Sigmoid_266              1 1 505_splitexcalibur_1 506",
                    "BinaryOp         Mul_267                  2 1 505_splitexcalibur_0 506 507 0=2",
                    "Convolution      Conv_268                 1 1 507 508 0=288 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=746496",
                    "Split            splitexcalibur_91        1 2 508 508_splitexcalibur_0 508_splitexcalibur_1",
                    "Sigmoid          Sigmoid_269              1 1 508_splitexcalibur_1 509",
                    "BinaryOp         Mul_270                  2 1 508_splitexcalibur_0 509 510 0=2",
                    "Convolution      Conv_271                 1 1 495_splitexcalibur_0 511 0=288 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=331776",
                    "Split            splitexcalibur_92        1 2 511 511_splitexcalibur_0 511_splitexcalibur_1",
                    "Sigmoid          Sigmoid_272              1 1 511_splitexcalibur_1 512",
                    "BinaryOp         Mul_273                  2 1 511_splitexcalibur_0 512 513 0=2",
                    "Concat           Concat_274               2 1 510 513 514 0=-1",
                    "Convolution      Conv_275                 1 1 514 515 0=576 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=331776",
                    "Split            splitexcalibur_93        1 2 515 515_splitexcalibur_0 515_splitexcalibur_1",
                    "Sigmoid          Sigmoid_276              1 1 515_splitexcalibur_1 516",
                    "BinaryOp         Mul_277                  2 1 515_splitexcalibur_0 516 517 0=2",
                    "Convolution      Conv_278                 1 1 517 518 0=384 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=221184",
                    "Split            splitexcalibur_94        1 2 518 518_splitexcalibur_0 518_splitexcalibur_1",
                    "Sigmoid          Sigmoid_279              1 1 518_splitexcalibur_1 519",
                    "BinaryOp         Mul_280                  2 1 518_splitexcalibur_0 519 520 0=2",
                    "Split            splitexcalibur_95        1 2 520 520_splitexcalibur_0 520_splitexcalibur_1",
                    "Interp           Resize_282               1 1 520_splitexcalibur_1 525 0=1 1=2.000000e+00 2=2.000000e+00 3=0 4=0 6=0",
                    "Concat           Concat_283               2 1 525 421_splitexcalibur_0 526 0=-1",
                    "Split            splitexcalibur_96        1 2 526 526_splitexcalibur_0 526_splitexcalibur_1",
                    "Convolution      Conv_284                 1 1 526_splitexcalibur_1 527 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=147456",
                    "Split            splitexcalibur_97        1 2 527 527_splitexcalibur_0 527_splitexcalibur_1",
                    "Sigmoid          Sigmoid_285              1 1 527_splitexcalibur_1 528",
                    "BinaryOp         Mul_286                  2 1 527_splitexcalibur_0 528 529 0=2",
                    "Convolution      Conv_287                 1 1 529 530 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=36864",
                    "Split            splitexcalibur_98        1 2 530 530_splitexcalibur_0 530_splitexcalibur_1",
                    "Sigmoid          Sigmoid_288              1 1 530_splitexcalibur_1 531",
                    "BinaryOp         Mul_289                  2 1 530_splitexcalibur_0 531 532 0=2",
                    "Convolution      Conv_290                 1 1 532 533 0=192 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=331776",
                    "Split            splitexcalibur_99        1 2 533 533_splitexcalibur_0 533_splitexcalibur_1",
                    "Sigmoid          Sigmoid_291              1 1 533_splitexcalibur_1 534",
                    "BinaryOp         Mul_292                  2 1 533_splitexcalibur_0 534 535 0=2",
                    "Convolution      Conv_293                 1 1 535 536 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=36864",
                    "Split            splitexcalibur_100       1 2 536 536_splitexcalibur_0 536_splitexcalibur_1",
                    "Sigmoid          Sigmoid_294              1 1 536_splitexcalibur_1 537",
                    "BinaryOp         Mul_295                  2 1 536_splitexcalibur_0 537 538 0=2",
                    "Convolution      Conv_296                 1 1 538 539 0=192 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=331776",
                    "Split            splitexcalibur_101       1 2 539 539_splitexcalibur_0 539_splitexcalibur_1",
                    "Sigmoid          Sigmoid_297              1 1 539_splitexcalibur_1 540",
                    "BinaryOp         Mul_298                  2 1 539_splitexcalibur_0 540 541 0=2",
                    "Convolution      Conv_299                 1 1 526_splitexcalibur_0 542 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=147456",
                    "Split            splitexcalibur_102       1 2 542 542_splitexcalibur_0 542_splitexcalibur_1",
                    "Sigmoid          Sigmoid_300              1 1 542_splitexcalibur_1 543",
                    "BinaryOp         Mul_301                  2 1 542_splitexcalibur_0 543 544 0=2",
                    "Concat           Concat_302               2 1 541 544 545 0=-1",
                    "Convolution      Conv_303                 1 1 545 546 0=384 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=147456",
                    "Split            splitexcalibur_103       1 2 546 546_splitexcalibur_0 546_splitexcalibur_1",
                    "Sigmoid          Sigmoid_304              1 1 546_splitexcalibur_1 547",
                    "BinaryOp         Mul_305                  2 1 546_splitexcalibur_0 547 548 0=2",
                    "Convolution      Conv_306                 1 1 548 549 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=73728",
                    "Split            splitexcalibur_104       1 2 549 549_splitexcalibur_0 549_splitexcalibur_1",
                    "Sigmoid          Sigmoid_307              1 1 549_splitexcalibur_1 550",
                    "BinaryOp         Mul_308                  2 1 549_splitexcalibur_0 550 551 0=2",
                    "Split            splitexcalibur_105       1 2 551 551_splitexcalibur_0 551_splitexcalibur_1",
                    "Interp           Resize_310               1 1 551_splitexcalibur_1 556 0=1 1=2.000000e+00 2=2.000000e+00 3=0 4=0 6=0",
                    "Concat           Concat_311               2 1 556 366_splitexcalibur_0 557 0=-1",
                    "Split            splitexcalibur_106       1 2 557 557_splitexcalibur_0 557_splitexcalibur_1",
                    "Convolution      Conv_312                 1 1 557_splitexcalibur_1 558 0=96 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=36864",
                    "Split            splitexcalibur_107       1 2 558 558_splitexcalibur_0 558_splitexcalibur_1",
                    "Sigmoid          Sigmoid_313              1 1 558_splitexcalibur_1 559",
                    "BinaryOp         Mul_314                  2 1 558_splitexcalibur_0 559 560 0=2",
                    "Convolution      Conv_315                 1 1 560 561 0=96 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=9216",
                    "Split            splitexcalibur_108       1 2 561 561_splitexcalibur_0 561_splitexcalibur_1",
                    "Sigmoid          Sigmoid_316              1 1 561_splitexcalibur_1 562",
                    "BinaryOp         Mul_317                  2 1 561_splitexcalibur_0 562 563 0=2",
                    "Convolution      Conv_318                 1 1 563 564 0=96 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=82944",
                    "Split            splitexcalibur_109       1 2 564 564_splitexcalibur_0 564_splitexcalibur_1",
                    "Sigmoid          Sigmoid_319              1 1 564_splitexcalibur_1 565",
                    "BinaryOp         Mul_320                  2 1 564_splitexcalibur_0 565 566 0=2",
                    "Convolution      Conv_321                 1 1 566 567 0=96 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=9216",
                    "Split            splitexcalibur_110       1 2 567 567_splitexcalibur_0 567_splitexcalibur_1",
                    "Sigmoid          Sigmoid_322              1 1 567_splitexcalibur_1 568",
                    "BinaryOp         Mul_323                  2 1 567_splitexcalibur_0 568 569 0=2",
                    "Convolution      Conv_324                 1 1 569 570 0=96 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=82944",
                    "Split            splitexcalibur_111       1 2 570 570_splitexcalibur_0 570_splitexcalibur_1",
                    "Sigmoid          Sigmoid_325              1 1 570_splitexcalibur_1 571",
                    "BinaryOp         Mul_326                  2 1 570_splitexcalibur_0 571 572 0=2",
                    "Convolution      Conv_327                 1 1 557_splitexcalibur_0 573 0=96 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=36864",
                    "Split            splitexcalibur_112       1 2 573 573_splitexcalibur_0 573_splitexcalibur_1",
                    "Sigmoid          Sigmoid_328              1 1 573_splitexcalibur_1 574",
                    "BinaryOp         Mul_329                  2 1 573_splitexcalibur_0 574 575 0=2",
                    "Concat           Concat_330               2 1 572 575 576 0=-1",
                    "Convolution      Conv_331                 1 1 576 577 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=36864",
                    "Split            splitexcalibur_113       1 2 577 577_splitexcalibur_0 577_splitexcalibur_1",
                    "Sigmoid          Sigmoid_332              1 1 577_splitexcalibur_1 578",
                    "BinaryOp         Mul_333                  2 1 577_splitexcalibur_0 578 579 0=2",
                    "Split            splitexcalibur_114       1 3 579 579_splitexcalibur_0 579_splitexcalibur_1 579_splitexcalibur_2",
                    "Convolution      Conv_334                 1 1 579_splitexcalibur_2 580 0=192 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=331776",
                    "Split            splitexcalibur_115       1 2 580 580_splitexcalibur_0 580_splitexcalibur_1",
                    "Sigmoid          Sigmoid_335              1 1 580_splitexcalibur_1 581",
                    "BinaryOp         Mul_336                  2 1 580_splitexcalibur_0 581 582 0=2",
                    "Concat           Concat_337               2 1 582 551_splitexcalibur_0 583 0=-1",
                    "Split            splitexcalibur_116       1 2 583 583_splitexcalibur_0 583_splitexcalibur_1",
                    "Convolution      Conv_338                 1 1 583_splitexcalibur_1 584 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=73728",
                    "Split            splitexcalibur_117       1 2 584 584_splitexcalibur_0 584_splitexcalibur_1",
                    "Sigmoid          Sigmoid_339              1 1 584_splitexcalibur_1 585",
                    "BinaryOp         Mul_340                  2 1 584_splitexcalibur_0 585 586 0=2",
                    "Convolution      Conv_341                 1 1 586 587 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=36864",
                    "Split            splitexcalibur_118       1 2 587 587_splitexcalibur_0 587_splitexcalibur_1",
                    "Sigmoid          Sigmoid_342              1 1 587_splitexcalibur_1 588",
                    "BinaryOp         Mul_343                  2 1 587_splitexcalibur_0 588 589 0=2",
                    "Convolution      Conv_344                 1 1 589 590 0=192 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=331776",
                    "Split            splitexcalibur_119       1 2 590 590_splitexcalibur_0 590_splitexcalibur_1",
                    "Sigmoid          Sigmoid_345              1 1 590_splitexcalibur_1 591",
                    "BinaryOp         Mul_346                  2 1 590_splitexcalibur_0 591 592 0=2",
                    "Convolution      Conv_347                 1 1 592 593 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=36864",
                    "Split            splitexcalibur_120       1 2 593 593_splitexcalibur_0 593_splitexcalibur_1",
                    "Sigmoid          Sigmoid_348              1 1 593_splitexcalibur_1 594",
                    "BinaryOp         Mul_349                  2 1 593_splitexcalibur_0 594 595 0=2",
                    "Convolution      Conv_350                 1 1 595 596 0=192 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=331776",
                    "Split            splitexcalibur_121       1 2 596 596_splitexcalibur_0 596_splitexcalibur_1",
                    "Sigmoid          Sigmoid_351              1 1 596_splitexcalibur_1 597",
                    "BinaryOp         Mul_352                  2 1 596_splitexcalibur_0 597 598 0=2",
                    "Convolution      Conv_353                 1 1 583_splitexcalibur_0 599 0=192 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=73728",
                    "Split            splitexcalibur_122       1 2 599 599_splitexcalibur_0 599_splitexcalibur_1",
                    "Sigmoid          Sigmoid_354              1 1 599_splitexcalibur_1 600",
                    "BinaryOp         Mul_355                  2 1 599_splitexcalibur_0 600 601 0=2",
                    "Concat           Concat_356               2 1 598 601 602 0=-1",
                    "Convolution      Conv_357                 1 1 602 603 0=384 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=147456",
                    "Split            splitexcalibur_123       1 2 603 603_splitexcalibur_0 603_splitexcalibur_1",
                    "Sigmoid          Sigmoid_358              1 1 603_splitexcalibur_1 604",
                    "BinaryOp         Mul_359                  2 1 603_splitexcalibur_0 604 605 0=2",
                    "Split            splitexcalibur_124       1 3 605 605_splitexcalibur_0 605_splitexcalibur_1 605_splitexcalibur_2",
                    "Convolution      Conv_360                 1 1 605_splitexcalibur_2 606 0=384 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=1327104",
                    "Split            splitexcalibur_125       1 2 606 606_splitexcalibur_0 606_splitexcalibur_1",
                    "Sigmoid          Sigmoid_361              1 1 606_splitexcalibur_1 607",
                    "BinaryOp         Mul_362                  2 1 606_splitexcalibur_0 607 608 0=2",
                    "Concat           Concat_363               2 1 608 520_splitexcalibur_0 609 0=-1",
                    "Split            splitexcalibur_126       1 2 609 609_splitexcalibur_0 609_splitexcalibur_1",
                    "Convolution      Conv_364                 1 1 609_splitexcalibur_1 610 0=288 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=221184",
                    "Split            splitexcalibur_127       1 2 610 610_splitexcalibur_0 610_splitexcalibur_1",
                    "Sigmoid          Sigmoid_365              1 1 610_splitexcalibur_1 611",
                    "BinaryOp         Mul_366                  2 1 610_splitexcalibur_0 611 612 0=2",
                    "Convolution      Conv_367                 1 1 612 613 0=288 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=82944",
                    "Split            splitexcalibur_128       1 2 613 613_splitexcalibur_0 613_splitexcalibur_1",
                    "Sigmoid          Sigmoid_368              1 1 613_splitexcalibur_1 614",
                    "BinaryOp         Mul_369                  2 1 613_splitexcalibur_0 614 615 0=2",
                    "Convolution      Conv_370                 1 1 615 616 0=288 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=746496",
                    "Split            splitexcalibur_129       1 2 616 616_splitexcalibur_0 616_splitexcalibur_1",
                    "Sigmoid          Sigmoid_371              1 1 616_splitexcalibur_1 617",
                    "BinaryOp         Mul_372                  2 1 616_splitexcalibur_0 617 618 0=2",
                    "Convolution      Conv_373                 1 1 618 619 0=288 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=82944",
                    "Split            splitexcalibur_130       1 2 619 619_splitexcalibur_0 619_splitexcalibur_1",
                    "Sigmoid          Sigmoid_374              1 1 619_splitexcalibur_1 620",
                    "BinaryOp         Mul_375                  2 1 619_splitexcalibur_0 620 621 0=2",
                    "Convolution      Conv_376                 1 1 621 622 0=288 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=746496",
                    "Split            splitexcalibur_131       1 2 622 622_splitexcalibur_0 622_splitexcalibur_1",
                    "Sigmoid          Sigmoid_377              1 1 622_splitexcalibur_1 623",
                    "BinaryOp         Mul_378                  2 1 622_splitexcalibur_0 623 624 0=2",
                    "Convolution      Conv_379                 1 1 609_splitexcalibur_0 625 0=288 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=221184",
                    "Split            splitexcalibur_132       1 2 625 625_splitexcalibur_0 625_splitexcalibur_1",
                    "Sigmoid          Sigmoid_380              1 1 625_splitexcalibur_1 626",
                    "BinaryOp         Mul_381                  2 1 625_splitexcalibur_0 626 627 0=2",
                    "Concat           Concat_382               2 1 624 627 628 0=-1",
                    "Convolution      Conv_383                 1 1 628 629 0=576 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=331776",
                    "Split            splitexcalibur_133       1 2 629 629_splitexcalibur_0 629_splitexcalibur_1",
                    "Sigmoid          Sigmoid_384              1 1 629_splitexcalibur_1 630",
                    "BinaryOp         Mul_385                  2 1 629_splitexcalibur_0 630 631 0=2",
                    "Split            splitexcalibur_134       1 3 631 631_splitexcalibur_0 631_splitexcalibur_1 631_splitexcalibur_2",
                    "Convolution      Conv_386                 1 1 631_splitexcalibur_2 632 0=576 1=3 11=3 2=1 12=1 3=2 13=2 4=1 14=1 15=1 16=1 5=1 6=2985984",
                    "Split            splitexcalibur_135       1 2 632 632_splitexcalibur_0 632_splitexcalibur_1",
                    "Sigmoid          Sigmoid_387              1 1 632_splitexcalibur_1 633",
                    "BinaryOp         Mul_388                  2 1 632_splitexcalibur_0 633 634 0=2",
                    "Concat           Concat_389               2 1 634 489_splitexcalibur_0 635 0=-1",
                    "Split            splitexcalibur_136       1 2 635 635_splitexcalibur_0 635_splitexcalibur_1",
                    "Convolution      Conv_390                 1 1 635_splitexcalibur_1 636 0=384 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=442368",
                    "Split            splitexcalibur_137       1 2 636 636_splitexcalibur_0 636_splitexcalibur_1",
                    "Sigmoid          Sigmoid_391              1 1 636_splitexcalibur_1 637",
                    "BinaryOp         Mul_392                  2 1 636_splitexcalibur_0 637 638 0=2",
                    "Convolution      Conv_393                 1 1 638 639 0=384 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=147456",
                    "Split            splitexcalibur_138       1 2 639 639_splitexcalibur_0 639_splitexcalibur_1",
                    "Sigmoid          Sigmoid_394              1 1 639_splitexcalibur_1 640",
                    "BinaryOp         Mul_395                  2 1 639_splitexcalibur_0 640 641 0=2",
                    "Convolution      Conv_396                 1 1 641 642 0=384 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=1327104",
                    "Split            splitexcalibur_139       1 2 642 642_splitexcalibur_0 642_splitexcalibur_1",
                    "Sigmoid          Sigmoid_397              1 1 642_splitexcalibur_1 643",
                    "BinaryOp         Mul_398                  2 1 642_splitexcalibur_0 643 644 0=2",
                    "Convolution      Conv_399                 1 1 644 645 0=384 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=147456",
                    "Split            splitexcalibur_140       1 2 645 645_splitexcalibur_0 645_splitexcalibur_1",
                    "Sigmoid          Sigmoid_400              1 1 645_splitexcalibur_1 646",
                    "BinaryOp         Mul_401                  2 1 645_splitexcalibur_0 646 647 0=2",
                    "Convolution      Conv_402                 1 1 647 648 0=384 1=3 11=3 2=1 12=1 3=1 13=1 4=1 14=1 15=1 16=1 5=1 6=1327104",
                    "Split            splitexcalibur_141       1 2 648 648_splitexcalibur_0 648_splitexcalibur_1",
                    "Sigmoid          Sigmoid_403              1 1 648_splitexcalibur_1 649",
                    "BinaryOp         Mul_404                  2 1 648_splitexcalibur_0 649 650 0=2",
                    "Convolution      Conv_405                 1 1 635_splitexcalibur_0 651 0=384 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=442368",
                    "Split            splitexcalibur_142       1 2 651 651_splitexcalibur_0 651_splitexcalibur_1",
                    "Sigmoid          Sigmoid_406              1 1 651_splitexcalibur_1 652",
                    "BinaryOp         Mul_407                  2 1 651_splitexcalibur_0 652 653 0=2",
                    "Concat           Concat_408               2 1 650 653 654 0=-1",
                    "Convolution      Conv_409                 1 1 654 655 0=768 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=589824",
                    "Split            splitexcalibur_143       1 2 655 655_splitexcalibur_0 655_splitexcalibur_1",
                    "Sigmoid          Sigmoid_410              1 1 655_splitexcalibur_1 656",
                    "BinaryOp         Mul_411                  2 1 655_splitexcalibur_0 656 657 0=2",
                    "Split            splitexcalibur_144       1 2 657 657_splitexcalibur_0 657_splitexcalibur_1",
                    "Convolution      Conv_412                 1 1 579_splitexcalibur_1 658 0=18 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=3456",
                    "Convolution      Conv_413                 1 1 579_splitexcalibur_0 659 0=126 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=24192",
                    "Concat           Concat_414               2 1 658 659 660 0=-1",
                    "Reshape          Reshape_428              1 1 660 678 0=6400 1=48 2=3",
                    "Transpose        Transpose_429            1 1 678 output 0=1,0,2",
                    "Convolution      Conv_430                 1 1 605_splitexcalibur_1 680 0=18 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=6912",
                    "Convolution      Conv_431                 1 1 605_splitexcalibur_0 681 0=126 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=48384",
                    "Concat           Concat_432               2 1 680 681 682 0=-1",
                    "Reshape          Reshape_446              1 1 682 700 0=1600 1=48 2=3",
                    "Transpose        Transpose_447            1 1 700 701 0=1,0,2",
                    "Convolution      Conv_448                 1 1 631_splitexcalibur_1 702 0=18 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=10368",
                    "Convolution      Conv_449                 1 1 631_splitexcalibur_0 703 0=126 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=72576",
                    "Concat           Concat_450               2 1 702 703 704 0=-1",
                    "Reshape          Reshape_464              1 1 704 722 0=400 1=48 2=3",
                    "Transpose        Transpose_465            1 1 722 723 0=1,0,2",
                    "Convolution      Conv_466                 1 1 657_splitexcalibur_1 724 0=18 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=13824",
                    "Convolution      Conv_467                 1 1 657_splitexcalibur_0 725 0=126 1=1 11=1 2=1 12=1 3=1 13=1 4=0 14=0 15=0 16=0 5=1 6=96768",
                    "Concat           Concat_468               2 1 724 725 726 0=-1",
                    "Reshape          Reshape_482              1 1 726 744 0=100 1=48 2=3",
                    "Transpose        Transpose_483            1 1 744 745 0=1,0,2"
            };
            
            inline static const std::vector<std::string> mobile_unicorn{
                "glsv1 Unicorn_Mobile_Net",
                "108 121",
                "Input            data             0 1 data 0=128 1=128 2=3 3=123.0,117.0,104.0 4=0.0078125",
                "Convolution      conv1            1 1 data conv1 0=64 1=3 2=1 3=2 4=1 5=1 6=1728",
                "PReLU            relu1            1 1 conv1 conv1_relu1 0=64",
                "ConvolutionDepthWise conv1_dw         1 1 conv1_relu1 conv1_dw 0=64 1=3 2=1 3=1 4=1 5=1 6=576 7=64",
                "PReLU            relu1_dw         1 1 conv1_dw conv1_dw_relu1_dw 0=64",
                "Convolution      conv2_ex         1 1 conv1_dw_relu1_dw conv2_ex 0=128 1=1 2=1 3=1 4=0 5=1 6=8192",
                "PReLU            relu2_ex         1 1 conv2_ex conv2_ex_relu2_ex 0=128",
                "ConvolutionDepthWise conv2_dw         1 1 conv2_ex_relu2_ex conv2_dw 0=128 1=3 2=1 3=2 4=1 5=1 6=1152 7=128",
                "PReLU            relu2_dw         1 1 conv2_dw conv2_dw_relu2_dw 0=128",
                "Convolution      conv2_em         1 1 conv2_dw_relu2_dw conv2_em 0=64 1=1 2=1 3=1 4=0 5=1 6=8192",
                "Split            conv2_em_conv2_em/scale_0_split 1 2 conv2_em conv2_em_conv2_em/scale_0_split_0 conv2_em_conv2_em/scale_0_split_1",
                "Convolution      conv2_1_ex       1 1 conv2_em_conv2_em/scale_0_split_0 conv2_1_ex 0=128 1=1 2=1 3=1 4=0 5=1 6=8192",
                "PReLU            relu2_1_ex       1 1 conv2_1_ex conv2_1_ex_relu2_1_ex 0=128",
                "ConvolutionDepthWise conv2_1_dw       1 1 conv2_1_ex_relu2_1_ex conv2_1_dw 0=128 1=3 2=1 3=1 4=1 5=1 6=1152 7=128",
                "PReLU            relu2_1_dw       1 1 conv2_1_dw conv2_1_dw_relu2_1_dw 0=128",
                "Convolution      conv2_1_em       1 1 conv2_1_dw_relu2_1_dw conv2_1_em 0=64 1=1 2=1 3=1 4=0 5=1 6=8192",
                "Eltwise          res2_1           2 1 conv2_em_conv2_em/scale_0_split_1 conv2_1_em res2_1 0=1 -23301=0",
                "Split            res2_1_res2_1_0_split 1 2 res2_1 res2_1_res2_1_0_split_0 res2_1_res2_1_0_split_1",
                "Convolution      conv2_2_ex       1 1 res2_1_res2_1_0_split_0 conv2_2_ex 0=128 1=1 2=1 3=1 4=0 5=1 6=8192",
                "PReLU            relu2_2_ex       1 1 conv2_2_ex conv2_2_ex_relu2_2_ex 0=128",
                "ConvolutionDepthWise conv2_2_dw       1 1 conv2_2_ex_relu2_2_ex conv2_2_dw 0=128 1=3 2=1 3=1 4=1 5=1 6=1152 7=128",
                "PReLU            relu2_2_dw       1 1 conv2_2_dw conv2_2_dw_relu2_2_dw 0=128",
                "Convolution      conv2_2_em       1 1 conv2_2_dw_relu2_2_dw conv2_2_em 0=64 1=1 2=1 3=1 4=0 5=1 6=8192",
                "Eltwise          res2_2           2 1 res2_1_res2_1_0_split_1 conv2_2_em res2_2 0=1 -23301=0",
                "Split            res2_2_res2_2_0_split 1 2 res2_2 res2_2_res2_2_0_split_0 res2_2_res2_2_0_split_1",
                "Convolution      conv2_3_ex       1 1 res2_2_res2_2_0_split_0 conv2_3_ex 0=128 1=1 2=1 3=1 4=0 5=1 6=8192",
                "PReLU            relu2_3_ex       1 1 conv2_3_ex conv2_3_ex_relu2_3_ex 0=128",
                "ConvolutionDepthWise conv2_3_dw       1 1 conv2_3_ex_relu2_3_ex conv2_3_dw 0=128 1=3 2=1 3=1 4=1 5=1 6=1152 7=128",
                "PReLU            relu2_3_dw       1 1 conv2_3_dw conv2_3_dw_relu2_3_dw 0=128",
                "Convolution      conv2_3_em       1 1 conv2_3_dw_relu2_3_dw conv2_3_em 0=64 1=1 2=1 3=1 4=0 5=1 6=8192",
                "Eltwise          res2_3           2 1 res2_2_res2_2_0_split_1 conv2_3_em res2_3 0=1 -23301=0",
                "Split            res2_3_res2_3_0_split 1 2 res2_3 res2_3_res2_3_0_split_0 res2_3_res2_3_0_split_1",
                "Convolution      conv2_4_ex       1 1 res2_3_res2_3_0_split_0 conv2_4_ex 0=128 1=1 2=1 3=1 4=0 5=1 6=8192",
                "PReLU            relu2_4_ex       1 1 conv2_4_ex conv2_4_ex_relu2_4_ex 0=128",
                "ConvolutionDepthWise conv2_4_dw       1 1 conv2_4_ex_relu2_4_ex conv2_4_dw 0=128 1=3 2=1 3=1 4=1 5=1 6=1152 7=128",
                "PReLU            relu2_4_dw       1 1 conv2_4_dw conv2_4_dw_relu2_4_dw 0=128",
                "Convolution      conv2_4_em       1 1 conv2_4_dw_relu2_4_dw conv2_4_em 0=64 1=1 2=1 3=1 4=0 5=1 6=8192",
                "Eltwise          res2_4           2 1 res2_3_res2_3_0_split_1 conv2_4_em res2_4 0=1 -23301=0",
                "Convolution      conv3_ex         1 1 res2_4 conv3_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=16384",
                "PReLU            relu3_ex         1 1 conv3_ex conv3_ex_relu3_ex 0=256",
                "ConvolutionDepthWise conv3_dw         1 1 conv3_ex_relu3_ex conv3_dw 0=256 1=3 2=1 3=2 4=1 5=1 6=2304 7=256",
                "PReLU            relu3_dw         1 1 conv3_dw conv3_dw_relu3_dw 0=256",
                "Convolution      conv3_em         1 1 conv3_dw_relu3_dw conv3_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768",
                "Split            conv3_em_conv3_em/scale_0_split 1 2 conv3_em conv3_em_conv3_em/scale_0_split_0 conv3_em_conv3_em/scale_0_split_1",
                "Convolution      conv3_1_ex       1 1 conv3_em_conv3_em/scale_0_split_0 conv3_1_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768",
                "PReLU            relu3_1_ex       1 1 conv3_1_ex conv3_1_ex_relu3_1_ex 0=256",
                "ConvolutionDepthWise conv3_1_dw       1 1 conv3_1_ex_relu3_1_ex conv3_1_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256",
                "PReLU            relu3_1_dw       1 1 conv3_1_dw conv3_1_dw_relu3_1_dw 0=256",
                "Convolution      conv3_1_em       1 1 conv3_1_dw_relu3_1_dw conv3_1_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768",
                "Eltwise          res3_1           2 1 conv3_em_conv3_em/scale_0_split_1 conv3_1_em res3_1 0=1 -23301=0",
                "Split            res3_1_res3_1_0_split 1 2 res3_1 res3_1_res3_1_0_split_0 res3_1_res3_1_0_split_1",
                "Convolution      conv3_2_ex       1 1 res3_1_res3_1_0_split_0 conv3_2_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768",
                "PReLU            relu3_2_ex       1 1 conv3_2_ex conv3_2_ex_relu3_2_ex 0=256",
                "ConvolutionDepthWise conv3_2_dw       1 1 conv3_2_ex_relu3_2_ex conv3_2_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256",
                "PReLU            relu3_2_dw       1 1 conv3_2_dw conv3_2_dw_relu3_2_dw 0=256",
                "Convolution      conv3_2_em       1 1 conv3_2_dw_relu3_2_dw conv3_2_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768",
                "Eltwise          res3_2           2 1 res3_1_res3_1_0_split_1 conv3_2_em res3_2 0=1 -23301=0",
                "Split            res3_2_res3_2_0_split 1 2 res3_2 res3_2_res3_2_0_split_0 res3_2_res3_2_0_split_1",
                "Convolution      conv3_3_ex       1 1 res3_2_res3_2_0_split_0 conv3_3_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768",
                "PReLU            relu3_3_ex       1 1 conv3_3_ex conv3_3_ex_relu3_3_ex 0=256",
                "ConvolutionDepthWise conv3_3_dw       1 1 conv3_3_ex_relu3_3_ex conv3_3_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256",
                "PReLU            relu3_3_dw       1 1 conv3_3_dw conv3_3_dw_relu3_3_dw 0=256",
                "Convolution      conv3_3_em       1 1 conv3_3_dw_relu3_3_dw conv3_3_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768",
                "Eltwise          res3_3           2 1 res3_2_res3_2_0_split_1 conv3_3_em res3_3 0=1 -23301=0",
                "Split            res3_3_res3_3_0_split 1 2 res3_3 res3_3_res3_3_0_split_0 res3_3_res3_3_0_split_1",
                "Convolution      conv3_4_ex       1 1 res3_3_res3_3_0_split_0 conv3_4_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768",
                "PReLU            relu3_4_ex       1 1 conv3_4_ex conv3_4_ex_relu3_4_ex 0=256",
                "ConvolutionDepthWise conv3_4_dw       1 1 conv3_4_ex_relu3_4_ex conv3_4_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256",
                "PReLU            relu3_4_dw       1 1 conv3_4_dw conv3_4_dw_relu3_4_dw 0=256",
                "Convolution      conv3_4_em       1 1 conv3_4_dw_relu3_4_dw conv3_4_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768",
                "Eltwise          res3_4           2 1 res3_3_res3_3_0_split_1 conv3_4_em res3_4 0=1 -23301=0",
                "Split            res3_4_res3_4_0_split 1 2 res3_4 res3_4_res3_4_0_split_0 res3_4_res3_4_0_split_1",
                "Convolution      conv3_5_ex       1 1 res3_4_res3_4_0_split_0 conv3_5_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768",
                "PReLU            relu3_5_ex       1 1 conv3_5_ex conv3_5_ex_relu3_5_ex 0=256",
                "ConvolutionDepthWise conv3_5_dw       1 1 conv3_5_ex_relu3_5_ex conv3_5_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256",
                "PReLU            relu3_5_dw       1 1 conv3_5_dw conv3_5_dw_relu3_5_dw 0=256",
                "Convolution      conv3_5_em       1 1 conv3_5_dw_relu3_5_dw conv3_5_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768",
                "Eltwise          res3_5           2 1 res3_4_res3_4_0_split_1 conv3_5_em res3_5 0=1 -23301=0",
                "Split            res3_5_res3_5_0_split 1 2 res3_5 res3_5_res3_5_0_split_0 res3_5_res3_5_0_split_1",
                "Convolution      conv3_6_ex       1 1 res3_5_res3_5_0_split_0 conv3_6_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768",
                "PReLU            relu3_6_ex       1 1 conv3_6_ex conv3_6_ex_relu3_6_ex 0=256",
                "ConvolutionDepthWise conv3_6_dw       1 1 conv3_6_ex_relu3_6_ex conv3_6_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256",
                "PReLU            relu3_6_dw       1 1 conv3_6_dw conv3_6_dw_relu3_6_dw 0=256",
                "Convolution      conv3_6_em       1 1 conv3_6_dw_relu3_6_dw conv3_6_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768",
                "Eltwise          res3_6           2 1 res3_5_res3_5_0_split_1 conv3_6_em res3_6 0=1 -23301=0",
                "Convolution      conv4_ex         1 1 res3_6 conv4_ex 0=512 1=1 2=1 3=1 4=0 5=1 6=65536",
                "PReLU            relu4_ex         1 1 conv4_ex conv4_ex_relu4_ex 0=512",
                "ConvolutionDepthWise conv4_dw         1 1 conv4_ex_relu4_ex conv4_dw 0=512 1=3 2=1 3=2 4=1 5=1 6=4608 7=512",
                "PReLU            relu4_dw         1 1 conv4_dw conv4_dw_relu4_dw 0=512",
                "Convolution      conv4_em         1 1 conv4_dw_relu4_dw conv4_em 0=128 1=1 2=1 3=1 4=0 5=1 6=65536",
                "Split            conv4_em_conv4_em/scale_0_split 1 2 conv4_em conv4_em_conv4_em/scale_0_split_0 conv4_em_conv4_em/scale_0_split_1",
                "Convolution      conv4_1_ex       1 1 conv4_em_conv4_em/scale_0_split_0 conv4_1_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768",
                "PReLU            relu4_1_ex       1 1 conv4_1_ex conv4_1_ex_relu4_1_ex 0=256",
                "ConvolutionDepthWise conv4_1_dw       1 1 conv4_1_ex_relu4_1_ex conv4_1_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256",
                "PReLU            relu4_1_dw       1 1 conv4_1_dw conv4_1_dw_relu4_1_dw 0=256",
                "Convolution      conv4_1_em       1 1 conv4_1_dw_relu4_1_dw conv4_1_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768",
                "Eltwise          res4_1           2 1 conv4_em_conv4_em/scale_0_split_1 conv4_1_em res4_1 0=1 -23301=0",
                "Split            res4_1_res4_1_0_split 1 2 res4_1 res4_1_res4_1_0_split_0 res4_1_res4_1_0_split_1",
                "Convolution      conv4_2_ex       1 1 res4_1_res4_1_0_split_0 conv4_2_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768",
                "PReLU            relu4_2_ex       1 1 conv4_2_ex conv4_2_ex_relu4_2_ex 0=256",
                "ConvolutionDepthWise conv4_2_dw       1 1 conv4_2_ex_relu4_2_ex conv4_2_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256",
                "PReLU            relu4_2_dw       1 1 conv4_2_dw conv4_2_dw_relu4_2_dw 0=256",
                "Convolution      conv4_2_em       1 1 conv4_2_dw_relu4_2_dw conv4_2_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768",
                "Eltwise          res4_2           2 1 res4_1_res4_1_0_split_1 conv4_2_em res4_2 0=1 -23301=0",
                "Convolution      conv5_ex         1 1 res4_2 conv5_ex 0=512 1=1 2=1 3=1 4=0 5=1 6=65536",
                "PReLU            relu5_ex         1 1 conv5_ex conv5_ex_relu5_ex 0=512",
                "ConvolutionDepthWise conv5_dw         1 1 conv5_ex_relu5_ex conv5_dw 0=512 1=8 2=1 3=1 4=0 5=1 6=32768 7=512",
                "InnerProduct     fc5              1 1 conv5_dw fc5 0=128 1=0 2=65536"};

            inline static const std::vector<std::string> mobile_unicorn_int8{
                "glsv1 Unicorn_Mobile_INT8_Net",
                "108 121",
                "Input            data             0 1 data 0=128 1=128 2=3 3=104.0,114.0,127.0 4=0.0078125",
                "Convolution      conv1            1 1 data conv1 0=64 1=3 2=1 3=2 4=1 5=1 6=1728 8=2",
                "PReLU            relu1            1 1 conv1 conv1_relu1 0=64",
                "ConvolutionDepthWise conv1_dw         1 1 conv1_relu1 conv1_dw 0=64 1=3 2=1 3=1 4=1 5=1 6=576 7=64 8=1",
                "PReLU            relu1_dw         1 1 conv1_dw conv1_dw_relu1_dw 0=64",
                "Convolution      conv2_ex         1 1 conv1_dw_relu1_dw conv2_ex 0=128 1=1 2=1 3=1 4=0 5=1 6=8192 8=2",
                "PReLU            relu2_ex         1 1 conv2_ex conv2_ex_relu2_ex 0=128",
                "ConvolutionDepthWise conv2_dw         1 1 conv2_ex_relu2_ex conv2_dw 0=128 1=3 2=1 3=2 4=1 5=1 6=1152 7=128 8=1",
                "PReLU            relu2_dw         1 1 conv2_dw conv2_dw_relu2_dw 0=128",
                "Convolution      conv2_em         1 1 conv2_dw_relu2_dw conv2_em 0=64 1=1 2=1 3=1 4=0 5=1 6=8192 8=2",
                "Split            conv2_em_conv2_em/scale_0_split 1 2 conv2_em conv2_em_conv2_em/scale_0_split_0 conv2_em_conv2_em/scale_0_split_1",
                "Convolution      conv2_1_ex       1 1 conv2_em_conv2_em/scale_0_split_0 conv2_1_ex 0=128 1=1 2=1 3=1 4=0 5=1 6=8192 8=2",
                "PReLU            relu2_1_ex       1 1 conv2_1_ex conv2_1_ex_relu2_1_ex 0=128",
                "ConvolutionDepthWise conv2_1_dw       1 1 conv2_1_ex_relu2_1_ex conv2_1_dw 0=128 1=3 2=1 3=1 4=1 5=1 6=1152 7=128 8=1",
                "PReLU            relu2_1_dw       1 1 conv2_1_dw conv2_1_dw_relu2_1_dw 0=128",
                "Convolution      conv2_1_em       1 1 conv2_1_dw_relu2_1_dw conv2_1_em 0=64 1=1 2=1 3=1 4=0 5=1 6=8192 8=2",
                "Eltwise          res2_1           2 1 conv2_em_conv2_em/scale_0_split_1 conv2_1_em res2_1 0=1 -23301=0",
                "Split            res2_1_res2_1_0_split 1 2 res2_1 res2_1_res2_1_0_split_0 res2_1_res2_1_0_split_1",
                "Convolution      conv2_2_ex       1 1 res2_1_res2_1_0_split_0 conv2_2_ex 0=128 1=1 2=1 3=1 4=0 5=1 6=8192 8=2",
                "PReLU            relu2_2_ex       1 1 conv2_2_ex conv2_2_ex_relu2_2_ex 0=128",
                "ConvolutionDepthWise conv2_2_dw       1 1 conv2_2_ex_relu2_2_ex conv2_2_dw 0=128 1=3 2=1 3=1 4=1 5=1 6=1152 7=128 8=1",
                "PReLU            relu2_2_dw       1 1 conv2_2_dw conv2_2_dw_relu2_2_dw 0=128",
                "Convolution      conv2_2_em       1 1 conv2_2_dw_relu2_2_dw conv2_2_em 0=64 1=1 2=1 3=1 4=0 5=1 6=8192 8=2",
                "Eltwise          res2_2           2 1 res2_1_res2_1_0_split_1 conv2_2_em res2_2 0=1 -23301=0",
                "Split            res2_2_res2_2_0_split 1 2 res2_2 res2_2_res2_2_0_split_0 res2_2_res2_2_0_split_1",
                "Convolution      conv2_3_ex       1 1 res2_2_res2_2_0_split_0 conv2_3_ex 0=128 1=1 2=1 3=1 4=0 5=1 6=8192 8=2",
                "PReLU            relu2_3_ex       1 1 conv2_3_ex conv2_3_ex_relu2_3_ex 0=128",
                "ConvolutionDepthWise conv2_3_dw       1 1 conv2_3_ex_relu2_3_ex conv2_3_dw 0=128 1=3 2=1 3=1 4=1 5=1 6=1152 7=128 8=1",
                "PReLU            relu2_3_dw       1 1 conv2_3_dw conv2_3_dw_relu2_3_dw 0=128",
                "Convolution      conv2_3_em       1 1 conv2_3_dw_relu2_3_dw conv2_3_em 0=64 1=1 2=1 3=1 4=0 5=1 6=8192 8=2",
                "Eltwise          res2_3           2 1 res2_2_res2_2_0_split_1 conv2_3_em res2_3 0=1 -23301=0",
                "Split            res2_3_res2_3_0_split 1 2 res2_3 res2_3_res2_3_0_split_0 res2_3_res2_3_0_split_1",
                "Convolution      conv2_4_ex       1 1 res2_3_res2_3_0_split_0 conv2_4_ex 0=128 1=1 2=1 3=1 4=0 5=1 6=8192 8=2",
                "PReLU            relu2_4_ex       1 1 conv2_4_ex conv2_4_ex_relu2_4_ex 0=128",
                "ConvolutionDepthWise conv2_4_dw       1 1 conv2_4_ex_relu2_4_ex conv2_4_dw 0=128 1=3 2=1 3=1 4=1 5=1 6=1152 7=128 8=1",
                "PReLU            relu2_4_dw       1 1 conv2_4_dw conv2_4_dw_relu2_4_dw 0=128",
                "Convolution      conv2_4_em       1 1 conv2_4_dw_relu2_4_dw conv2_4_em 0=64 1=1 2=1 3=1 4=0 5=1 6=8192 8=2",
                "Eltwise          res2_4           2 1 res2_3_res2_3_0_split_1 conv2_4_em res2_4 0=1 -23301=0",
                "Convolution      conv3_ex         1 1 res2_4 conv3_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=16384 8=2",
                "PReLU            relu3_ex         1 1 conv3_ex conv3_ex_relu3_ex 0=256",
                "ConvolutionDepthWise conv3_dw         1 1 conv3_ex_relu3_ex conv3_dw 0=256 1=3 2=1 3=2 4=1 5=1 6=2304 7=256 8=1",
                "PReLU            relu3_dw         1 1 conv3_dw conv3_dw_relu3_dw 0=256",
                "Convolution      conv3_em         1 1 conv3_dw_relu3_dw conv3_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "Split            conv3_em_conv3_em/scale_0_split 1 2 conv3_em conv3_em_conv3_em/scale_0_split_0 conv3_em_conv3_em/scale_0_split_1",
                "Convolution      conv3_1_ex       1 1 conv3_em_conv3_em/scale_0_split_0 conv3_1_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "PReLU            relu3_1_ex       1 1 conv3_1_ex conv3_1_ex_relu3_1_ex 0=256",
                "ConvolutionDepthWise conv3_1_dw       1 1 conv3_1_ex_relu3_1_ex conv3_1_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256 8=1",
                "PReLU            relu3_1_dw       1 1 conv3_1_dw conv3_1_dw_relu3_1_dw 0=256",
                "Convolution      conv3_1_em       1 1 conv3_1_dw_relu3_1_dw conv3_1_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "Eltwise          res3_1           2 1 conv3_em_conv3_em/scale_0_split_1 conv3_1_em res3_1 0=1 -23301=0",
                "Split            res3_1_res3_1_0_split 1 2 res3_1 res3_1_res3_1_0_split_0 res3_1_res3_1_0_split_1",
                "Convolution      conv3_2_ex       1 1 res3_1_res3_1_0_split_0 conv3_2_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "PReLU            relu3_2_ex       1 1 conv3_2_ex conv3_2_ex_relu3_2_ex 0=256",
                "ConvolutionDepthWise conv3_2_dw       1 1 conv3_2_ex_relu3_2_ex conv3_2_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256 8=1",
                "PReLU            relu3_2_dw       1 1 conv3_2_dw conv3_2_dw_relu3_2_dw 0=256",
                "Convolution      conv3_2_em       1 1 conv3_2_dw_relu3_2_dw conv3_2_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "Eltwise          res3_2           2 1 res3_1_res3_1_0_split_1 conv3_2_em res3_2 0=1 -23301=0",
                "Split            res3_2_res3_2_0_split 1 2 res3_2 res3_2_res3_2_0_split_0 res3_2_res3_2_0_split_1",
                "Convolution      conv3_3_ex       1 1 res3_2_res3_2_0_split_0 conv3_3_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "PReLU            relu3_3_ex       1 1 conv3_3_ex conv3_3_ex_relu3_3_ex 0=256",
                "ConvolutionDepthWise conv3_3_dw       1 1 conv3_3_ex_relu3_3_ex conv3_3_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256 8=1",
                "PReLU            relu3_3_dw       1 1 conv3_3_dw conv3_3_dw_relu3_3_dw 0=256",
                "Convolution      conv3_3_em       1 1 conv3_3_dw_relu3_3_dw conv3_3_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "Eltwise          res3_3           2 1 res3_2_res3_2_0_split_1 conv3_3_em res3_3 0=1 -23301=0",
                "Split            res3_3_res3_3_0_split 1 2 res3_3 res3_3_res3_3_0_split_0 res3_3_res3_3_0_split_1",
                "Convolution      conv3_4_ex       1 1 res3_3_res3_3_0_split_0 conv3_4_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "PReLU            relu3_4_ex       1 1 conv3_4_ex conv3_4_ex_relu3_4_ex 0=256",
                "ConvolutionDepthWise conv3_4_dw       1 1 conv3_4_ex_relu3_4_ex conv3_4_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256 8=1",
                "PReLU            relu3_4_dw       1 1 conv3_4_dw conv3_4_dw_relu3_4_dw 0=256",
                "Convolution      conv3_4_em       1 1 conv3_4_dw_relu3_4_dw conv3_4_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "Eltwise          res3_4           2 1 res3_3_res3_3_0_split_1 conv3_4_em res3_4 0=1 -23301=0",
                "Split            res3_4_res3_4_0_split 1 2 res3_4 res3_4_res3_4_0_split_0 res3_4_res3_4_0_split_1",
                "Convolution      conv3_5_ex       1 1 res3_4_res3_4_0_split_0 conv3_5_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "PReLU            relu3_5_ex       1 1 conv3_5_ex conv3_5_ex_relu3_5_ex 0=256",
                "ConvolutionDepthWise conv3_5_dw       1 1 conv3_5_ex_relu3_5_ex conv3_5_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256 8=1",
                "PReLU            relu3_5_dw       1 1 conv3_5_dw conv3_5_dw_relu3_5_dw 0=256",
                "Convolution      conv3_5_em       1 1 conv3_5_dw_relu3_5_dw conv3_5_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "Eltwise          res3_5           2 1 res3_4_res3_4_0_split_1 conv3_5_em res3_5 0=1 -23301=0",
                "Split            res3_5_res3_5_0_split 1 2 res3_5 res3_5_res3_5_0_split_0 res3_5_res3_5_0_split_1",
                "Convolution      conv3_6_ex       1 1 res3_5_res3_5_0_split_0 conv3_6_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "PReLU            relu3_6_ex       1 1 conv3_6_ex conv3_6_ex_relu3_6_ex 0=256",
                "ConvolutionDepthWise conv3_6_dw       1 1 conv3_6_ex_relu3_6_ex conv3_6_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256 8=1",
                "PReLU            relu3_6_dw       1 1 conv3_6_dw conv3_6_dw_relu3_6_dw 0=256",
                "Convolution      conv3_6_em       1 1 conv3_6_dw_relu3_6_dw conv3_6_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "Eltwise          res3_6           2 1 res3_5_res3_5_0_split_1 conv3_6_em res3_6 0=1 -23301=0",
                "Convolution      conv4_ex         1 1 res3_6 conv4_ex 0=512 1=1 2=1 3=1 4=0 5=1 6=65536 8=2",
                "PReLU            relu4_ex         1 1 conv4_ex conv4_ex_relu4_ex 0=512",
                "ConvolutionDepthWise conv4_dw         1 1 conv4_ex_relu4_ex conv4_dw 0=512 1=3 2=1 3=2 4=1 5=1 6=4608 7=512 8=1",
                "PReLU            relu4_dw         1 1 conv4_dw conv4_dw_relu4_dw 0=512",
                "Convolution      conv4_em         1 1 conv4_dw_relu4_dw conv4_em 0=128 1=1 2=1 3=1 4=0 5=1 6=65536 8=2",
                "Split            conv4_em_conv4_em/scale_0_split 1 2 conv4_em conv4_em_conv4_em/scale_0_split_0 conv4_em_conv4_em/scale_0_split_1",
                "Convolution      conv4_1_ex       1 1 conv4_em_conv4_em/scale_0_split_0 conv4_1_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "PReLU            relu4_1_ex       1 1 conv4_1_ex conv4_1_ex_relu4_1_ex 0=256",
                "ConvolutionDepthWise conv4_1_dw       1 1 conv4_1_ex_relu4_1_ex conv4_1_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256 8=1",
                "PReLU            relu4_1_dw       1 1 conv4_1_dw conv4_1_dw_relu4_1_dw 0=256",
                "Convolution      conv4_1_em       1 1 conv4_1_dw_relu4_1_dw conv4_1_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "Eltwise          res4_1           2 1 conv4_em_conv4_em/scale_0_split_1 conv4_1_em res4_1 0=1 -23301=0",
                "Split            res4_1_res4_1_0_split 1 2 res4_1 res4_1_res4_1_0_split_0 res4_1_res4_1_0_split_1",
                "Convolution      conv4_2_ex       1 1 res4_1_res4_1_0_split_0 conv4_2_ex 0=256 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "PReLU            relu4_2_ex       1 1 conv4_2_ex conv4_2_ex_relu4_2_ex 0=256",
                "ConvolutionDepthWise conv4_2_dw       1 1 conv4_2_ex_relu4_2_ex conv4_2_dw 0=256 1=3 2=1 3=1 4=1 5=1 6=2304 7=256 8=1",
                "PReLU            relu4_2_dw       1 1 conv4_2_dw conv4_2_dw_relu4_2_dw 0=256",
                "Convolution      conv4_2_em       1 1 conv4_2_dw_relu4_2_dw conv4_2_em 0=128 1=1 2=1 3=1 4=0 5=1 6=32768 8=2",
                "Eltwise          res4_2           2 1 res4_1_res4_1_0_split_1 conv4_2_em res4_2 0=1 -23301=0",
                "Convolution      conv5_ex         1 1 res4_2 conv5_ex 0=512 1=1 2=1 3=1 4=0 5=1 6=65536 8=2",
                "PReLU            relu5_ex         1 1 conv5_ex conv5_ex_relu5_ex 0=512",
                "ConvolutionDepthWise conv5_dw         1 1 conv5_ex_relu5_ex conv5_dw 0=512 1=8 2=1 3=1 4=0 5=1 6=32768 7=512 8=1",
                "InnerProduct     fc5              1 1 conv5_dw fc5 0=128 1=0 2=65536"};

        };

        const std::unordered_map<std::string, std::vector<std::string>> hardcode_map{
            {"meter_sim", hardcode_model_params::meter_sim},
			{"posture", hardcode_model_params::posture},            
            {"mobile_unicorn", hardcode_model_params::mobile_unicorn},
            {"mobile_unicorn_int8", hardcode_model_params::mobile_unicorn_int8},
            {"mobile_unicorn_mask", hardcode_model_params::mobile_unicorn},
            {"mobile_unicorn_mask_int8", hardcode_model_params::mobile_unicorn_int8},
        };
    }

    std::vector<std::string> get_model_params(std::string_view name, bool use_int8)
    {
        auto iter = use_int8 ? hardcode_map.find(std::string(name) + "_int8") : hardcode_map.find(std::string(name));

        return iter != hardcode_map.end() ? iter->second : std::vector<std::string>();
    }
}
