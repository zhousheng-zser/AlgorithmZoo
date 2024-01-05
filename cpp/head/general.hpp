
    struct safe_crop_rect 
    {
        int x1;
        int x2;
        int y1;
        int y2;

        safe_crop_rect(int x11,int x22,int y11,int y22,int width,int height)
        {
            x1 = x11>0 ? x11:0;
            x2 = x22>0 ? x22:0;
            y1 = y11>0 ? y11:0;
            y2 = y22>0 ? y22:0;

            x1 = x1 <width? x1 : width;
            x2 = x2 <width? x2 : width;
            y1 = y1 <height? y1 : height;
            y2 = y2 <height? y2 : height;

        }

    };

    
    static inline float sigmoid_x(float x)
    {
        return static_cast<float>(1.f / (1.f + exp(-x)));
    }

    void tranpose(const float* sou, float* dest, int sourows, int soucols)
    {
        for(int i=0;i< sourows;i++)
            for(int j=0;j< soucols;j++)
                dest[j*sourows+i]=sou[ i * soucols + j];    
    }

    void  Softmax(float* data, int num )
    {             
        double L2_Sum=0.f;
        for(size_t i=0; i<num; i++) 
        {
            data[i]= ( exp(data[i] ) );
            L2_Sum +=  data[i];
        }
        for(size_t i=0; i<num; i++) 
            data[i] =  data[i] / L2_Sum ;
    }

    inline float de_sigmoid(float x)
    {
        if(x>=1 ||x<0)
            return NAN;
        return static_cast<float> (log( x/(1-x)));
    }