
#ifndef  _MEMBOOKS_H_
#define _MEMBOOKS_H_

#include "database/bookSql.hpp"
#include "rapidfuzz/fuzz.hpp"
#include "rapidfuzz/utils.hpp"
#include <string>
#include <pthread.h>
#include <list>
#include <map>
#include <vector>
#include <iostream>

using std::cout;
using std::endl ;
using std::map ;
using rapidfuzz::fuzz::ratio;
using std::vector;
using std::list ;
using std::string ;
using std::pair;

//转大写
string to_upper(const string & str)
{
    string buffer = str;
    int len = buffer.length();
    for(int index = 0; index < len; index++){
        if( 'a' <= buffer[index] && buffer[index]<= 'z'){
            buffer[index] = buffer[index] + 'A' -'a';
        }
    }
    return buffer;
}


//将书籍的作者、书名合并
//其中以\t分割
class  infoCombineBook
{
public:
    infoCombineBook(){}
    ~infoCombineBook(){}
    infoCombineBook(const string & book_id,const int & auto_book_id,const string & combine_info ) 
    {
        __bookId = book_id;
        __combineInfo = combine_info;
        __autoBookId = auto_book_id;
    };
    //combineInfo 格式一定是作者\t书名
    infoCombineBook(BookBaseInfoTable && book) 
    {
        __bookId = std::move(book.bookId );
        __autoBookId = std::move(book.autoBookId);

        //数据块空信息处理
        if(book.authorName == "")  book.authorName = "  ";
        if(book.bookName == "")  book.bookName = "  ";


        __combineInfo = book.authorName + "\t" + book.bookName  ;

    };
    void getTranslateBook(BookBaseInfoTable & book)
    {//转换回正常的书籍信息
        book.bookId = __bookId ;


        int lastPos = 0 ,currentPos = -1  ;

        //切割
        currentPos =  __combineInfo.find("\t",currentPos + 1) ;
        book.authorName = std::move(__combineInfo.substr(lastPos ,currentPos - lastPos  ) );
        lastPos = currentPos ;

        book.bookName = std::move(__combineInfo.substr(lastPos+ 1,currentPos - lastPos -1 ) );

    };
    int getautoBookId()
    {
        return __autoBookId;
    }
    //返回组合信息
    string getCombineInfo(){return __combineInfo;};
    string getBookId(){return __bookId;};
private:
    int __autoBookId;         // 主键
    string __bookId;
    string __combineInfo;   //用于搜索的信息
};

class locker
{
public:
    locker()
    {
        if (pthread_mutex_init(&__mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&__mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&__mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&__mutex) == 0;
    }
    pthread_mutex_t *get()
    {
        return &__mutex;
    }

private:
    pthread_mutex_t __mutex;
};

class MemBooks
{
public:
    MemBooks(){};
    ~MemBooks(){};
    void setBooks(vector<CombineBook> && books){
        
        for(auto & book : books){
            __books.push_back(std::move(infoCombineBook(std::move(get<0>(book) ))) );
        }
    }
    int insertBook(BookBaseInfoTable && book)
    {
        __locker.lock();
        list<infoCombineBook>::iterator it ,end ;
        it = __books.begin();
        end = __books.end();
        int result = 1 ;//默认插入成功
        for(;it != end;it++){
            if(it->getBookId() == book.bookId){
                result = -1 ;//已经有该书籍
                __locker.unlock();
                return result;
            }
        }
        //实例更新
        __books.push_back(std::move(infoCombineBook(std::move(book) ) ));
        __locker.unlock();
        return result; //成功
    };
    int deleteBook(const string & bookId)
    {
        __locker.lock();
        list<infoCombineBook>::iterator it ,end ;
        it = __books.begin();
        end = __books.end();
        int result = 1 ;//默认删除成功
        for(;it != end;it++){
            if(it->getBookId() == bookId){
                __books.erase(it);
                __locker.unlock();
                return result;
            }
        }
        __locker.unlock();
        result = -1 ;
        return result ;//查找不到返回
    };
    int updateBook(const BookBaseInfoTable & book)
    {
        return 0 ;
    }
    //模糊匹配对应本数,对应起点的书籍
    int  fuzzySearch(vector<int> & auto_book_id ,const string & words ,
            const int & offset,const int & count)
    {
        //参数检查
        if(words == "" || offset < 0 || count <= 0){
            return -1;
        }

        __locker.lock();
        map<int,vector<infoCombineBook> > buffer ;//匹配结果
        list<infoCombineBook>::iterator it ,end ;
        it = __books.begin();
        end = __books.end();

        map<int,vector<infoCombineBook>>::reverse_iterator rBufBgein ;

        int score ;
        cout<<"  internal size is "<<__books.size()<<endl;
        for(;it != end;it++){
            score = (int)(10 * rapidfuzz::fuzz::partial_ratio(to_upper(it->getCombineInfo()),to_upper(words) ) );
            cout<<"  ratio  score "<<score<<endl;
            if(score >= 400){
                buffer[score].push_back(*it);
            }
        }
        
        //根据偏移量获取起始点
        rBufBgein = buffer.rbegin();
        for(int index = 0 ; index < offset ;index++){
            if(rBufBgein == buffer.rend()){
                __locker.unlock();
                return 1;
            }
            rBufBgein++;
        }
        cout<<" buf  size " <<buffer.size()<<"   count " <<count<<" is "<< (rBufBgein == buffer.rend())<<endl;
        //偏移后添加
        BookBaseInfoTable book ;
        for(int index = 0 ;index < count && rBufBgein != buffer.rend() ;index++,rBufBgein++ ){
            cout<<" score  is "<<rBufBgein->first<<endl ;

            for(auto & item :rBufBgein->second){
                auto_book_id.push_back(item.getautoBookId());
            }
        }
        __locker.unlock();
        return 1;
    };
    int  size(){
        return (int)__books.size();
    }
private:
    locker __locker; //加锁对于实例化书籍进行保护
    list<infoCombineBook>__books;
};


#endif
