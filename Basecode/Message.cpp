#include"Message.h"
#include<cstdio>

void Message::save_msg()
{
    //发送时，把对象转为流对象，然后转为char*
    boost::archive::text_oarchive oa(ss); 
    oa << *this; 
}

void Message::load_msg()
{
    boost::archive::text_iarchive ia(ss);  
    ia >> *this; 
}

char * Message::send_content( char * buf , size_t size)
{
    save_msg();
    ss.getline(buf , size);
    return buf;
}

void Message::read_content(char* buf , size_t size)
{
    ss.clear();
    auto len = strlen(buf);
    ss.write(buf , len);
    load_msg();
}
