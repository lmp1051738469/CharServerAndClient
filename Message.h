#include<string>
//add message  proto
class Message
{
private:
    int id;
    std::string msg;
public:
    Message() = default;
    Message(int id_ , std::string msg_) : id(id_) , msg(msg_){}
    ~Message() = default;
    const char* str_c()const {return msg.c_str()};
    int get_id()const {return id;}
};


