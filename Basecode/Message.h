#include <boost/archive/text_oarchive.hpp> 
#include <boost/archive/text_iarchive.hpp> 
#include <boost/serialization/string.hpp> 
#include<string>
#include<sstream>

#ifndef _MESSAGE_H_
#define _MESSAGE_H_

class Message
{
private:
friend class boost::serialization::access; 
    int id;
    std::string content;
  template <typename Archive> 
  friend void serialize(Archive &ar, Message &p, const unsigned int version)
  {
    ar & p.id; 
    ar & p.content; 
  }
    void save_msg(std::stringstream &);
    void load_msg(std::stringstream &);
public:
    Message() = default;
    Message(int id_ , std::string msg_) : id(id_) , content(msg_){}
    ~Message() = default;
    const char* str_c()const {return content.c_str();};
    int get_id()const {return id;}

    //构建字节流， 把字符流转成char * , 
    void send_content( std::stringstream &sa);

    //把char* 转换成字符流 ， 构建对象
    void read_content(std::stringstream &sa );
};

#endif


