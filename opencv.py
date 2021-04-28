#!/usr/bin/python
# Warning:
# Works with OpenCV4
# Does NOT work with OpenCV3

# Basic idea
# We crop the image first.
# Crop image into upper 3 rings, and lower 3 objects.
#   PS: Do the deep copy `crop_img = img[y:y+h, x:x+w].copy()`
#       Not the `crop_img = img[y:y+h, x:x+w]`
# Then, the upper layer use `new_r = ((r - b) + (r - g))/2`
# to get rid of white, extract the color we want.
# The 4 yellow tapes at the corner is annoying, make sure they are cropped out.

# As for the lower part, background isn't white. try it at the field

# import packages
import cv2
import numpy as np
from pyzbar.pyzbar import decode
from enum import Enum


class Crop:
    def __init__(self, y1, y2, x1, x2):
        self.y = y1
        self.y_h = y2
        self.x = x1
        self.x_w = x2

    y = 0
    y_h = 0
    x = 0
    x_w = 0


CycCrop = Crop(220, 350, 400, 920)
RecCrop = Crop(471, 522, 774, 1040)


class CGs:
    RedX = 0
    GreenX = 0
    BlueX = 0


CycCGs = CGs()
RecCGs = CGs()


class MissionType_t(Enum):
    Unknown = 0
    BRG = 1
    GBR = 2
    RGB = 3


# All status of the status machine
class Status_t(Enum):
    start = 0
    Cropping = 3
    findQRCode = 1
    findCGofCyc = 2
    findCGofRec = 5
    TCP2DownMachine = 8
    Finished = 9


Status = Status_t.start
# Status = Status_t.findCGofCyc
# Status = Status_t.findCGofRec
# Status = Status_t.Cropping
MissionType = MissionType_t.Unknown

cap = cv2.VideoCapture("/dev/video2")
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)


def getOrder(vR,vG,vB):
    ret = ''
    if vR == min(vR, vB, vG):
        ret += 'R'
    elif vG == min(vR, vB, vG):
        ret += 'G'
    elif vB == min(vR, vB, vG):
        ret += 'B'

    if min(vR, vB, vG) < vR < max(vR, vB, vG):
        ret += 'R'
    elif min(vR, vB, vG) < vG < max(vR, vB, vG):
        ret += 'G'
    elif min(vR, vB, vG) < vB < max(vR, vB, vG):
        ret += 'B'

    if vR == max(vR, vB, vG):
        ret += 'R'
    elif vG == max(vR, vB, vG):
        ret += 'G'
    elif vB == max(vR, vB, vG):
        ret += 'B'
    return ret
    
    

def extractColor(var_r, var_g, var_b):
    var_b = var_b.astype(np.int16)
    var_g = var_g.astype(np.int16)
    var_r = var_r.astype(np.int16)
    # FIXME
    # here should we use abs((r - b) + (r - g))
    # or abs(r - b) + abs(r - g)?
    ret_r = (var_r - var_b) + (var_r - var_g)
    ret_r = ret_r / 2
    ret_r = ret_r.astype(np.int16)
    ret_r[ret_r < 0] = 0
    ret_r[ret_r > 255] = 255
    ret_r = ret_r.astype(np.uint8)

    ret_g = (var_g - var_r) + (var_g - var_b)
    ret_g = ret_g / 2
    ret_g = ret_g.astype(np.int16)
    ret_g[ret_g < 0] = 0
    ret_g[ret_g > 255] = 255
    ret_g = ret_g.astype(np.uint8)

    ret_b = (var_b - var_r) + (var_b - var_g)
    ret_b = ret_b / 2
    ret_b = ret_b.astype(np.int16)
    ret_b[ret_b < 0] = 0
    ret_b[ret_b > 255] = 255
    ret_b = ret_b.astype(np.uint8)
    return ret_r, ret_g, ret_b


if __name__ == "__main__":
    while Status != Status_t.Finished:
        if Status == Status_t.start:
            # FIXME Init all stuff
            # Drop first 10 frames to avoid rubbish
            ret, img = cap.read()
            while img is None:
                print("Rubbish")
                ret, img = cap.read()
            Status = Status_t.findQRCode
            pass
        elif Status == Status_t.Cropping:
            img = cv2.imread("a.jpg")
            # ret, img = cap.read()
            # img = img[CycCrop.y:CycCrop.y_h, CycCrop.x:CycCrop.x_w]
            cv2.imshow("image", img)
            key = cv2.waitKey(1)
            if key == ord("q"):
                break
        elif Status == Status_t.findQRCode:
            qrCodeDetector = cv2.QRCodeDetector()
            ret, img = cap.read()
            cv2.imshow("original", img)
            cv2.waitKey(1)
            gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
            for barcode in decode(gray):
                print(barcode.data.decode('utf-8'))
                if barcode.data.decode('utf-8') == "蓝-红-绿":
                    MissionType = MissionType_t.BRG
                    Status = Status_t.findCGofCyc
                elif barcode.data.decode('utf-8') == "绿-蓝-红":
                    MissionType = MissionType_t.GBR
                    Status = Status_t.findCGofCyc
                elif barcode.data.decode('utf-8') == "红-绿-蓝":
                    MissionType = MissionType_t.GBR
                    Status = Status_t.findCGofCyc
                    pass
                else:
                    # FIXME What if there were another QR code on the Filed?
                    raise ValueError('Check your Mission Type')
                pass
            pass
        elif Status == Status_t.findCGofCyc:
            # ret, image = cap.read()
            image = cv2.imread("a.jpg")
            crop = image[CycCrop.y:CycCrop.y_h, CycCrop.x:CycCrop.x_w].copy()
            b, g, r = crop[:, :, 0], crop[:, :, 1], crop[:, :, 2]
            new_r, new_g, new_b = extractColor(r, g, b)

            if CycCGs.RedX == 0:
                # thresh = cv2.adaptiveThreshold(new_r, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY, 3, 2)
                retVal, thresh = cv2.threshold(new_r, 40, 255, cv2.THRESH_BINARY)
                thresh = cv2.medianBlur(thresh, 5)
            elif CycCGs.GreenX == 0:
                # thresh = cv2.adaptiveThreshold(new_r, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY, 3, 2)
                retVal, thresh = cv2.threshold(new_g, 20, 255, cv2.THRESH_BINARY)
                # thresh = cv2.medianBlur(thresh, 5)
            elif CycCGs.BlueX == 0:
                # thresh = cv2.adaptiveThreshold(new_r, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY, 3, 2)
                retVal, thresh = cv2.threshold(new_b, 20, 255, cv2.THRESH_BINARY)
                # thresh = cv2.medianBlur(thresh, 5)
            else:
                print("CycRed:{}".format(CycCGs.RedX))
                print("CycGreen:{}".format(CycCGs.GreenX))
                print("CycBlue:{}".format(CycCGs.BlueX))
                Status = Status_t.findCGofRec
                continue

            contours, hierarchy = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)

            if contours:
                # find the biggest countour (c) by the area
                c = max(contours, key=cv2.contourArea)
                # print(cv2.contourArea(c))
                if cv2.contourArea(c) > 50:
                    M = cv2.moments(c)
                    cx = int(M['m10'] / M['m00'])
                    if CycCGs.RedX == 0:
                        CycCGs.RedX = cx
                    elif CycCGs.GreenX == 0:
                        CycCGs.GreenX = cx
                    elif CycCGs.BlueX == 0:
                        CycCGs.BlueX = cx
                    pass
                else:
                    # FIXME If the area of c too small, we are not detecting the right thing. Drop this frame.
                    pass
                # How to calculate CG of contours
                # https://docs.opencv.org/master/dd/d49/tutorial_py_contour_features.html

            # cv2.imshow("crop", crop)
            # cv2.imshow("new_b", new_b)
            # cv2.imshow("new_r", new_r)
            # cv2.imshow("new_g", new_g)
            # cv2.imshow("original", image)
            # cv2.imshow("thresh", thresh)
            # cv2.waitKey(1)


            # cap.release()
            # cv2.destroyAllWindows()
            pass
        elif Status == Status_t.findCGofRec:
            # ret, image = cap.read()
            image = cv2.imread("a.jpg")
            crop = image[RecCrop.y:RecCrop.y_h, RecCrop.x:RecCrop.x_w].copy()
            b, g, r = crop[:, :, 0], crop[:, :, 1], crop[:, :, 2]
            new_r, new_g, new_b = extractColor(r, g, b)

            if RecCGs.RedX == 0:
                # thresh = cv2.adaptiveThreshold(new_r, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY, 3, 2)
                retVal, thresh = cv2.threshold(new_r, 40, 255, cv2.THRESH_BINARY)
                thresh = cv2.medianBlur(thresh, 5)
            elif RecCGs.GreenX == 0:
                # thresh = cv2.adaptiveThreshold(new_r, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY, 3, 2)
                retVal, thresh = cv2.threshold(new_g, 20, 255, cv2.THRESH_BINARY)
                # thresh = cv2.medianBlur(thresh, 5)
            elif RecCGs.BlueX == 0:
                # thresh = cv2.adaptiveThreshold(new_r, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY, 3, 2)
                retVal, thresh = cv2.threshold(new_b, 20, 255, cv2.THRESH_BINARY)
                # thresh = cv2.medianBlur(thresh, 5)
            else:
                print("RecRed:{}".format(RecCGs.RedX))
                print("RecGreen:{}".format(RecCGs.GreenX))
                print("RecBlue:{}".format(RecCGs.BlueX))
                Status = Status_t.TCP2DownMachine
                continue

            contours, hierarchy = cv2.findContours(thresh, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)

            if contours:
                # find the biggest countour (c) by the area
                c = max(contours, key=cv2.contourArea)
                # print(cv2.contourArea(c))
                if cv2.contourArea(c) > 50:
                    M = cv2.moments(c)
                    cx = int(M['m10'] / M['m00'])
                    if RecCGs.RedX == 0:
                        RecCGs.RedX = cx
                    elif RecCGs.GreenX == 0:
                        RecCGs.GreenX = cx
                    elif RecCGs.BlueX == 0:
                        RecCGs.BlueX = cx
                    pass
                else:
                    # FIXME If the area of c too small, we are not detecting the right thing. Drop this frame.
                    pass
                # How to calculate CG of contours
                # https://docs.opencv.org/master/dd/d49/tutorial_py_contour_features.html

            cv2.imshow("crop", crop)
            # cv2.imshow("new_b", new_b)
            # cv2.imshow("new_r", new_r)
            # cv2.imshow("new_g", new_g)
            cv2.imshow("original", image)
            # cv2.imshow("thresh", thresh)
            cv2.waitKey(1)

            pass
        elif Status == Status_t.TCP2DownMachine:
            print("圆环左到右顺序为{}".format(getOrder(CycCGs.RedX, CycCGs.GreenX, CycCGs.BlueX)))
            print("物品左到右顺序为{}".format(getOrder(RecCGs.RedX, RecCGs.GreenX, RecCGs.BlueX)))
            Status = Status_t.Finished





# 霍夫变换
# https://opencv-python-tutroals.readthedocs.io/en/latest/py_tutorials/py_imgproc/py_houghlines/py_houghlines.html

# #中值滤波
# import cv2
# import numpy as np
# from matplotlib import pyplot as plt
# import matplotlib
# matplotlib.use('GTK3Agg')
# img = cv2.imread('/home/kisonhe/Downloads/wiki.jpg')
#
#
# median = cv2.medianBlur(img,5)
#
#
# plt.subplot(121),plt.imshow(img),plt.title('Original')
# plt.xticks([]), plt.yticks([])
# plt.subplot(122),plt.imshow(median),plt.title('Blurred')
# plt.xticks([]), plt.yticks([])
# plt.show()


# #高斯滤波
# import cv2
# import numpy as np
# from matplotlib import pyplot as plt
# import matplotlib
# matplotlib.use('GTK3Agg')
# img = cv2.imread('/home/kisonhe/Downloads/wiki.jpg')
#
#
# blur = cv2.GaussianBlur(img,(5,5),0)
#
#
# plt.subplot(121),plt.imshow(img),plt.title('Original')
# plt.xticks([]), plt.yticks([])
# plt.subplot(122),plt.imshow(blur),plt.title('Blurred')
# plt.xticks([]), plt.yticks([])
# plt.show()


# #均值滤波
# import cv2
# import numpy as np
# from matplotlib import pyplot as plt
# import matplotlib
# matplotlib.use('GTK3Agg')
# img = cv2.imread('/home/kisonhe/Downloads/wiki.jpg')
#
# blur = cv2.blur(img,(5,5))
#
# plt.subplot(121),plt.imshow(img),plt.title('Original')
# plt.xticks([]), plt.yticks([])
# plt.subplot(122),plt.imshow(blur),plt.title('Blurred')
# plt.xticks([]), plt.yticks([])
# plt.show()


# # 直方图
# # https://zh.wikipedia.org/wiki/%E7%9B%B4%E6%96%B9%E5%9B%BE%E5%9D%87%E8%A1%A1%E5%8C%96
# import cv2
# import numpy as np
# from matplotlib import pyplot as plt
# import matplotlib
# # Manjaro linux, qt seems broken even the libxcb and pyqt5 and all the dependencies
# # I can find on the internet
# # Has to use GTK, ****
# matplotlib.use('GTK3Agg')
# img = cv2.imread('/home/kisonhe/Downloads/wiki.jpg',0)
#
# hist,bins = np.histogram(img.flatten(),256,[0,256])
#
# cdf = hist.cumsum()
# cdf_normalized = cdf * hist.max()/ cdf.max()
#
# plt.plot(cdf_normalized, color = 'b')
# plt.hist(img.flatten(),256,[0,256], color = 'r')
# plt.xlim([0,256])
# plt.legend(('cdf','histogram'), loc = 'upper left')
# plt.show()


# # gamma
# import cv2
# import numpy as np
#
# gammaValue = 1
#
# def on_trackbar(val):
#     global gammaValue
#     gammaValue = val / 10
#     # beta = ( 1.0 - alpha )
#     # dst = cv.addWeighted(src1, alpha, src2, beta, 0.0)
#     # cv.imshow(title_window, dst)
#
#
# def adjust_gamma(image, gamma=1.0):
#     # 取倒数
#     invGamma = 1.0 / gamma
#     # 算
#     table = np.array([((i / 255.0) ** invGamma) * 255
#                       for i in np.arange(0, 256)]).astype("uint8")
#     return cv2.LUT(image, table)
#
#
# cap = cv2.VideoCapture("/dev/video2")
# cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
# cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)
#
# cv2.namedWindow("image")
# ret, img = cap.read()
# cv2.createTrackbar("gamma", "image" , 1, 100, on_trackbar)
# while True:
#     ret, img = cap.read()
#     img = adjust_gamma(img, gammaValue)
#     cv2.imshow("image", img)
#     key = cv2.waitKey(1)
#     if key == ord("q"):
#         break


# #灰度 and 二值化
# import cv2
# import numpy as np
# from PIL import Image
# #Linux 为 /dev/videoX
# #Windows下请自行修改为对应的摄像头
# cap = cv2.VideoCapture("/dev/video2")
# cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
# cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)
#
# thresh = 128
# while True:
#     ret, img = cap.read()
#     r, g, b = img[:, :, 0], img[:, :, 1], img[:, :, 2]
#     #直接取单通道就近似是个灰度了
#     # gray = 0.2989 * r + 0.5870 * g + 0.1140 * b
#     # gray = np.round(gray)
#     # gray = gray.astype(int)
#     cv2.imshow("image", r)
#     key = cv2.waitKey(0)
#     if key == ord("q"):
#         break
#     #对三个通道，如果某个数大于thresh就弄成1*255，否则为0*255
#     #也就是对三个通道二值化
#     binary = (img > thresh) * 255
#     binary = Image.fromarray(np.uint8(binary))
#     open_cv_binary = np.array(binary)
#     cv2.imshow("image", open_cv_binary)
#     key = cv2.waitKey(0)
#     if key == ord("q"):
#         break


# import cv2
# import numpy as np
# from pyzbar.pyzbar import decode
#
# alpha_slider_max = 500
# title_window = 'Linear Blend'
# img = 0
# gammavalue = 1
# def on_trackbar(val):
#     alpha = val / 100.0
#     gammavalue = alpha
#
# def gamma_trans(img, gamma):
#         gamma_table=[np.power(x/255.0,gamma)*255.0 for x in range(256)]
#         gamma_table=np.round(np.array(gamma_table)).astype(np.uint8)
#         return cv2.LUT(img,gamma_table)
#
#
# cv2.namedWindow("image")
#
# trackbar_name = 'Alpha x %d' % alpha_slider_max
# cv2.createTrackbar(trackbar_name, "image" , 0, alpha_slider_max, on_trackbar)
# # Show some stuff
# # on_trackbar(0)
#
# qrCodeDetector = cv2.QRCodeDetector()
#
# cap = cv2.VideoCapture("/dev/video2")
# cap.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
# cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)
# cnt = 0
# while True:
#     ret, img = cap.read()
#     gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
#     gray = gamma_trans(gray ,gammavalue)
#
#
#     for barcode in decode(gray):
#         # print(barcode.data)
#         print(cnt)
#         cnt+=1
#         print(barcode.data.decode('utf-8'))
#     cv2.imshow("image", gray)
#
#     key = cv2.waitKey(1)
#     if key == ord("q"):
#         break
#
# cap.release()
# cv2.destroyAllWindows()
