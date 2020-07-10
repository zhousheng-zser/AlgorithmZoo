# -*- coding: utf-8 -*-


# =========== text detection config parameters ===================

data_shape = 640

# train config
gpu_id = 0

# net config
backbone = 'resnet50'
Lambda = 0.7
# 尺度放缩的个数
n = 6
m = 0.5
OHEM_ratio = 3
scale = 1
# random seed
seed = 2

psenet_model_path = r'./models/PSENET.pth'

# =========== text recognition config parameters =================

labelPath = r'./dataset/label.txt'
# the height and width of the input image to network
imgH = 32
imgW = 100
# size of the lstm hidden state
nh = 256
cuda = True
# whether to keep ratio for image resize
keep_ratio = False
# reproduce experiemnt
manualSeed = 1234
# whether to sample the dataset with random sampler
random_sample = True

#  返回label
def get_label(path):
    with open(path, 'r', encoding='UTF-8') as f:
        label = f.read()
    label = label.split('\n')
    # 删除label的第0个元素‘blank'
    del label[0]
    label = ''.join(label)
    return label

alphabet = get_label(labelPath)

crnn_model_path = r'./models/CRNN.pth'
