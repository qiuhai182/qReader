#ifndef  _BOOKTYPE_H_
#define _BOOKTYPE_H_

#include <string>

namespace bookType{


    
    enum class primaryClass{
        Computer = 1 ,  
        Education= 2,
        Finance_economics = 3, 
        Science_fiction = 4,
        Suspense_reasoning = 5,//悬疑推理
        Romance = 6 ,//言情
        Literature = 7,
        History = 8,
        Geographic = 9,
        Political = 10,
        Chemical = 11,
        Biological = 12,
        Physical = 13,
        Mathematics = 14
    };
    
    std::string primary_enum_to_string(const primaryClass & type) 
    {

        switch(type){
            case primaryClass::Computer: 
                return std::string("计算机与互联网");
            case primaryClass::Education: 
                return std::string("教育");
            case primaryClass::Finance_economics: 
                return std::string("经管理财");
            case primaryClass::Science_fiction: 
                return std::string("科幻玄幻");
            case primaryClass::Suspense_reasoning: 
                return std::string("悬疑推理");
            case primaryClass::Romance: 
                return std::string("言情");
            case primaryClass::Literature: 
                return std::string("文学");
            case primaryClass::History: 
                return std::string("历史");
            case primaryClass::Geographic: 
                return std::string("地理");
            case primaryClass::Political: 
                return std::string("政治");
            case primaryClass::Chemical: 
                return std::string("化学");
            case primaryClass::Biological:
                return std::string("生物");
            case primaryClass::Physical: 
                return std::string("物理");
            case primaryClass::Mathematics: 
                return std::string("数学");
            default:
                return std::string("");
        }
    }

    int primary_string_to_int(const std::string & type)
    {
        if(type == "计算机与互联网"){
            return static_cast<int>(primaryClass::Computer);
        }else if(type == "教育"){
            return static_cast<int>(primaryClass::Education);
        }else if(type == "经管理财"){
            return static_cast<int>(primaryClass::Finance_economics);
        }else if(type == "科幻奇幻"){
            return static_cast<int>(primaryClass::Science_fiction);
        }else if(type == "悬疑推理"){
            return static_cast<int>(primaryClass::Suspense_reasoning);
        }else if(type == "言情"){
            return static_cast<int>(primaryClass::Romance);
        }else if(type == "文学"){
            return static_cast<int>(primaryClass::Literature);
        }else if(type == "历史"){
            return static_cast<int>(primaryClass::History);
        }else if(type == "地理"){
            return static_cast<int>(primaryClass::Geographic);
        }else if(type == "政治"){
            return static_cast<int>(primaryClass::Political);
        }else if(type == "化学"){
            return static_cast<int>(primaryClass::Chemical);
        }else if(type == "生物"){
            return static_cast<int>(primaryClass::Biological);
        }else if(type == "物理"){
            return static_cast<int>(primaryClass::Physical);
        }else if(type == "数学"){
            return static_cast<int>(primaryClass::Mathematics);
        }else{
            true -1;
        }
        
    }

    bool isPrimaryClass(int type)
    {
        if(type < 0 || type > 14){
            return false; 
        }
        return true;
    }

}
#endif