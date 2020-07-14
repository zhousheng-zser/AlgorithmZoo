#acc:94.5%
import torch
from torch import nn
class SeModule(nn.Module):
    '''
        insize:输入特征图通道数
        reduction:通道数缩放倍数
    '''
    def __init__(self, in_size, expand, reduction=4):  #in_size 为输入通道数,re
        super(SeModule, self).__init__()
        self.in_size = in_size*expand
        self.se = nn.Sequential(
            nn.AdaptiveAvgPool2d(1),
            nn.Conv2d(self.in_size, self.in_size // reduction, kernel_size=1, stride=1, padding=0, bias=False),
            nn.BatchNorm2d(self.in_size // reduction),
            nn.PReLU(),
            nn.Conv2d(self.in_size // reduction, self.in_size, kernel_size=1, stride=1, padding=0, bias=False),
            nn.BatchNorm2d(self.in_size)
        )
    def forward(self, x):
        return x * torch.sigmoid(self.se(x))


class MBblock(nn.Module):
    def __init__(self,input, output, ker_size, stride, expand_ratio, semodule):
        super(MBblock,self).__init__()
        self.inp = input  #输入尺寸
        self.oup = output   #输出尺寸
        self.s = stride    #步幅大小
        self.ke = ker_size  #卷积和大小
        self.expand = expand_ratio  #扩张通道，增加特征数
        self.exp_channel = input*expand_ratio  #扩张后的通道数
        self.pad = int((ker_size-1)/2)         #pad大小
        self.se = semodule                #是否使用semodule /通道注意力

        if expand_ratio!=1:
            self.conv1 = nn.Conv2d(self.inp, self.exp_channel, stride=1, kernel_size=1)
            self.bn1 = nn.BatchNorm2d(self.exp_channel)

        self.conv2 = nn.Conv2d(self.exp_channel, self.exp_channel, kernel_size=self.ke, stride= self.s, padding=self.pad, groups=self.exp_channel)
        self.bn2 = nn.BatchNorm2d(self.exp_channel)
        self.prelu = nn.PReLU()

        self.conv3 = nn.Conv2d(self.exp_channel, self.oup, kernel_size=1)
        self.bn3 = nn.BatchNorm2d(self.oup)

    def forward(self, x):
        input_x = x
        if self.expand !=1:
            x =self.prelu(self.bn1(self.conv1(x)))
        x = self.prelu(self.bn2(self.conv2(x)))
        if self.se != None:
            x = self.se(x)
        x = self.bn3(self.conv3(x))
        if self.inp == self.oup and self.s==1:
            x = x+input_x
        return x
class MLblock(nn.Module):
    def __init__(self, input, output, ker_size, stride, expand_ratio, semodule):
        super(MLblock, self).__init__()
        self.inp = input  #输入尺寸
        self.oup = output   #输出尺寸
        self.s = stride    #步幅大小
        self.ke = ker_size  #卷积和大小
        self.expand = expand_ratio  #扩张通道，增加特征数
        self.sca_channel = int(input*0.5)
        self.main_exp_channel = input*(expand_ratio-1)
        self.exp_channel = input*expand_ratio  #扩张后的通道数
        self.pad = int((ker_size-1)/2)         #pad大小
        self.se = semodule                #是否使用semodule /通道注意力

        if expand_ratio!=1:
            self.conv_w = nn.Conv2d(self.inp, self.sca_channel, kernel_size=(3, 9), stride=1, groups=self.sca_channel, padding=(1, 4))
            self.bnw = nn.BatchNorm2d(self.sca_channel)
            self.conv_l = nn.Conv2d(self.inp, self.sca_channel, kernel_size=(9, 3), stride=1, groups=self.sca_channel, padding=(4, 1))
            self.bnl = nn.BatchNorm2d(self.sca_channel)
            self.conv1 = nn.Conv2d(self.inp, self.main_exp_channel, stride=1, kernel_size=1)
            self.bn1 = nn.BatchNorm2d(self.main_exp_channel)

        self.conv2 = nn.Conv2d(self.exp_channel, self.exp_channel, kernel_size=self.ke, stride= self.s, padding=self.pad, groups=self.exp_channel)
        self.bn2 = nn.BatchNorm2d(self.exp_channel)
        self.prelu = nn.PReLU()

        self.conv3 = nn.Conv2d(self.exp_channel, self.oup, kernel_size=1)
        self.bn3 = nn.BatchNorm2d(self.oup)

    def forward(self, x):
        input_x = x
        if self.expand !=1:
            x1 =self.prelu(self.bn1(self.conv1(x)))

            x2 = self.prelu(self.bnl(self.conv_l(x)))
            x3 = self.prelu(self.bnw(self.conv_w(x)))
            x = torch.cat((x1, x2, x3), dim=1)
        x = self.prelu(self.bn2(self.conv2(x)))
        if self.se != None:
            x = self.se(x)
        x = self.bn3(self.conv3(x))
        if self.inp == self.oup and self.s==1:
            x = x+input_x
        return x
class Re_attention1(nn.Module):
    def __init__(self, input_channel, output_channel, ration):
        super(Re_attention1, self).__init__()

        self.average_pool = nn.AdaptiveAvgPool2d(1)
        self.pool = nn.MaxPool2d(kernel_size=ration, stride=ration)
        self.change_channel = nn.Conv2d(input_channel, output_channel, kernel_size=1, stride=1)
        self.bn = nn.BatchNorm2d(output_channel)

    def forward(self, x):
        x = self.bn(self.change_channel(self.pool(x)))
        y = self.average_pool(x)
        total = x*torch.sigmoid(y)
        return total

class Unicorn_compound7_longconv(nn.Module):
    def __init__(self, num_class):
        super(Unicorn_compound7_longconv, self).__init__()
        self.num_class = num_class
        self.conv1 = nn.Sequential(
            nn.Conv2d(1, 64, kernel_size=3, stride=1, padding=1),
            nn.BatchNorm2d(64),
            nn.Tanh())
        self.head = nn.Sequential(
            MLblock(64, 64, 5, 2, 1, None),
            MLblock(64, 64, 5, 1, 2, None),
            MLblock(64, 96, 3, 1, 2, None)
            )
        self.seq1 = nn.Sequential(
            MLblock(96, 96, 5, 2, 4, None),
            MLblock(96, 96, 5, 1, 3, None),
            MLblock(96, 96, 3, 1, 3, None),
            MLblock(96, 96, 3, 1, 3, None),
            MLblock(96, 96, 5, 1, 3, None)
            )
        self.seq2 = nn.Sequential(
            MLblock(96, 128, 5, 2, 4, None),
            MLblock(128, 128, 5, 1, 3, None),
            MLblock(128, 128, 5, 1, 3, None),
            MBblock(128, 128, 3, 1, 3, SeModule(128, 3)),
            MBblock(128, 128, 3, 1, 3, SeModule(128, 3)),
            MBblock(128, 128, 3, 1, 3, SeModule(128, 3)),
            MBblock(128, 128, 5, 1, 3, None),
            MBblock(128, 128, 5, 1, 3, None),
            )
        self.seq3 = nn.Sequential(
            MBblock(128, 160, 5, 2, 4, None),
            MBblock(160, 160, 5, 1, 3, None),
            MBblock(160, 160, 3, 1, 3, SeModule(160, 3)),
            MBblock(160, 160, 3, 1, 3, SeModule(160, 3))
            )

        self.tail = nn.Sequential(
            nn.Conv2d(160, 160, kernel_size=8, stride=1, groups=160),
            nn.BatchNorm2d(160),
            nn.PReLU(),
            nn.Conv2d(160, num_class, kernel_size=1, stride=1, bias=False)
        )
        self.ratio = nn.Sequential(
            nn.AdaptiveMaxPool2d(1),
            nn.Conv2d(160, 80, kernel_size=1, stride=1),
            nn.BatchNorm2d(80),
            nn.PReLU(),
            nn.Conv2d(80, 80, kernel_size=1, stride=1),
            nn.BatchNorm2d(80),
            nn.PReLU(),
            nn.Conv2d(80, num_class, kernel_size=1, stride=1, bias=False)
        )
        self.residual_0 = Re_attention1(64, 96, 2)
        self.residual_1 = Re_attention1(96, 96, 2)
        self.residual_2 = Re_attention1(96, 128, 2)
        self.residual_3 = Re_attention1(128, 160, 2)
    def forward(self, x):
        x = self.conv1(x)
        y = self.residual_0(x)
        x = self.head(x)
        y1 = self.residual_1(x)
        x = self.seq1(x+y)
        y = self.residual_2(x)
        x = self.seq2(x + y1)
        y1 = self.residual_3(x)
        x = self.seq3(x + y)
        h = self.ratio(x)
        x = self.tail(x + y1)
        x = x + h
        x = x.view(x.shape[0], x.shape[1])
        return x

if __name__=="__main__":
    import torch.nn as nn
    model = Unicorn_compound7_longconv(128)
    torch.save(model.state_dict(), '1.pth')
    semode = SeModule(64, 2)
    x = torch.randn(2, 1, 128, 128)
    # torch.load_state_dict(torch.load('./checkpoints/efficientnet_5.pth'))
    y = model(x)
    print(y.shape)




