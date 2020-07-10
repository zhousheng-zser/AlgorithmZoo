#!/usr/bin/env python3 
# -*- coding: utf-8 -*-

import numpy as np
import cv2
import os
from PIL import Image

import utils
import config
from psenet.psenet import PSENet
from crnn.crnn import CRNN
from psenet import predict as psenet_predict
from crnn import predict as crnn_predict

def text_detection(img):
    net = PSENet(backbone=config.backbone, pretrained=False, result_num=config.n)
    psenet_model = psenet_predict.PSENetHandel(config.psenet_model_path, net=net, scale=config.scale, gpu_id=config.gpu_id)
    preds, boxes_list, rects_rec, t = psenet_model.predict(img)
    return preds, boxes_list, rects_rec, t


def text_recognition(img, rects_re, f=1.0):
    results = []
    for index, rect in enumerate(rects_re):
        degree, w, h, cx, cy = rect
        # partImg, newW, newH = rotate_cut_img(im,  90  + degree  , cx, cy, w, h, leftAdjust, rightAdjust, alph)
        partImg = utils.crop_rect(img, ((cx, cy ),(h, w),degree))
        newH, newW = partImg.shape[0:2]
        cv2.imshow('img', partImg)
        cv2.waitKey(0)
        if newH > 1.5* newW:
            # 将矩阵逆时针旋转90度， -1为顺时针旋转
            partImg = np.rot90(partImg,1)
        partImg = cv2.cvtColor(partImg, cv2.COLOR_RGB2GRAY)

        # 加载crnn模型，获得识别结果
        net = CRNN(config.imgH, 1, len(config.alphabet) + 1, config.nh)
        crnn_model = crnn_predict.CRNNHandle(config.crnn_model_path, net=net, gpu_id=config.gpu_id)
        simPred = crnn_model.predict(partImg)
        # u表示将后面的字符以unicode格式编码
        if simPred.strip() != u'':
            results.append({'cx': cx * f, 'cy': cy * f, 'text': simPred, 'w': newW * f, 'h': newH * f,
                            'degree': degree })
    return results


def main():
    # 将此路径改为测试图片的路径
    img_path = './test_image/test8.jpg'
    img = cv2.imread(img_path)
    img = np.array(img)
    H, W = img.shape[0:2]

    preds, boxes_list, rects_re, t = text_detection(img)

    img2 = utils.draw_bbox(img, boxes_list, color=(0, 255, 0))
    cv2.imshow("image", img2)
    cv2.waitKey(0)

    results = text_recognition(np.array(img), rects_re)
    
    print('The recognition text is: %s' % results)

if __name__ == '__main__':
    main()