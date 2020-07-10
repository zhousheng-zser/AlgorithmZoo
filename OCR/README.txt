======= Environment ======
pytorch 1.2.0   
CUDA 10.0 + cudnn 7.5
python 3.6
widows10 (The program only run on win10)

======= File specification ======
angle: text angle prediction, will be completed in the future.
crnn:  text recognition algorithm(CRNN).
psenet: text detection algorithm(PSENet).
models: some training models of pytorch, include CRNN, PSENet and so on. Because the 
        pytorch model is larger, please put the downloaded .pth model into this file.
ocr_run: The ocr project main program. Only need config related test image path and 
         run the program, it will give detection and recognition results.
output: store the progrm's output.
utils.py: some auxiliary function

======= TO DO ======
1.Text angle prediction.
2.Use light weight model is CRNN and PSENet.
