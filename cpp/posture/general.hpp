      
    static inline float sigmoid_x(float x)
    {
        return static_cast<float>(1.f / (1.f + exp(-x)));
    }

    void tranpose(const float* sou, float* dest, int sourows, int soucols)
    {
        for(int i=0;i< sourows;i++)
        {
            for(int j=0;j< soucols;j++)
            {
                dest[j*sourows+i]=sou[ i * soucols + j];    
            }
        }
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
        {
            data[i] =  data[i] / L2_Sum ;
        }       
    }