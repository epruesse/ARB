#include <vector>

template <typename T>
class dynamic_array
{
public:
    dynamic_array(){};
    dynamic_array(int rows, int cols)
    {
        for(int i=0; i<rows; ++i){
            data_.push_back(std::vector<T>(cols));
        }
    }
    std::vector<T> & operator[](int i)
    {
        return data_[i];
    }
    const std::vector<T> & operator[] (int i) const
    {
        return data_[i];
    }

private:
    std::vector<std::vector<T> > data_;
    
}; 
