
#ifndef  _MEMBOOKS_H_
#define _MEMBOOKS_H_

#include "bookdata.hpp"
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

//将书籍的作者、简介、书名、类型合并
//其中以\t分割
class  infoCombineBook
{
public:
    infoCombineBook(){}
    ~infoCombineBook(){}
    infoCombineBook(const string & bookId, const string & combineInfo,
                    const string & bookHeadUrl,const string & bookDownUrl,
                    const int commentId ,const string & publishTime) 
    {
        m_bookId = bookId;
        m_combineInfo = combineInfo;
        m_bookHeadUrl = bookHeadUrl;
        m_bookDownUrl = bookDownUrl;
        m_commentId   = commentId;
        m_publishTime = publishTime;
    };
    //combineInfo 格式一定是作者\t简介\t书名\t类型
    infoCombineBook(BookInfoTable && book) 
    {
        m_bookId = std::move(book.bookId );
        //数据块空信息处理
        if(book.authorName == "")  book.authorName = "  ";
        if(book.bookIntro == "")  book.bookIntro = "  ";
        if(book.bookName == "")  book.bookName = "  ";
        if(book.bookType == "")  book.bookType = "  ";


        m_combineInfo = book.authorName + "\t" + book.bookIntro + "\t"
                        +book.bookName + "\t" + book.bookType ;
        m_bookHeadUrl = std::move(book.bookHeadUrl);
        m_bookDownUrl = std::move(book.bookDownUrl);
        m_commentId   = std::move(book.commentId);
        m_publishTime = std::move(book.publishTime);

    };
    void getTranslateBook(BookInfoTable & book)
    {//转换回正常的书籍信息
        book.bookId = m_bookId ;
        book.bookHeadUrl = m_bookHeadUrl;
        book.bookDownUrl = m_bookDownUrl;
        book.commentId = m_commentId   ;
        book.publishTime = m_publishTime ;
        int lastPos = 0 ,currentPos = -1  ;

        //切割
        currentPos =  m_combineInfo.find("\t",currentPos + 1) ;
        book.authorName = std::move(m_combineInfo.substr(lastPos ,currentPos - lastPos  ) );
        lastPos = currentPos ;

        currentPos =  m_combineInfo.find("\t",currentPos + 1);
        book.bookIntro = std::move(m_combineInfo.substr(lastPos+ 1,currentPos - lastPos -1 ) );
        lastPos = currentPos ;

        currentPos =  m_combineInfo.find("\t",currentPos + 1);
        book.bookName = std::move(m_combineInfo.substr(lastPos+ 1,currentPos - lastPos -1 ) );
        lastPos = currentPos ;

        book.bookType = std::move(m_combineInfo.substr(currentPos+ 1,m_combineInfo.size() - currentPos ) );

        cout<<" transform  is "<<book.authorName<<"  "<<book.bookIntro <<"    "<<book.bookName<<"   "<<book.bookType <<endl;
    };
    //返回组合信息
    string getCombineInfo(){return m_combineInfo;};
    string getBookId(){return m_bookId;};
private:
    string m_bookId;
    string m_combineInfo;   //用于搜索的信息
    // 其他信息
    string m_bookHeadUrl;		// 封面url
    string m_bookDownUrl;		// 下载地址
    int	   m_commentId;		// 首条评论id
    string m_publishTime;     // 出版时间
};

class locker
{
public:
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};

class MemBooks
{
public:
    MemBooks(){};
    ~MemBooks(){};
    void setBooks(vector<BookInfoTable> && books){
        for(auto & book : books){
            m_books.push_back(std::move(infoCombineBook(std::move(book)) ) );
        }
    }
    int insertBook(BookInfoTable && book)
    {
        m_locker.lock();
        list<infoCombineBook>::iterator it ,end ;
        it = m_books.begin();
        end = m_books.end();
        int result = 1 ;//默认插入成功
        for(;it != end;it++){
            if(it->getBookId() == book.bookId){
                result = -1 ;//已经有该书籍
                m_locker.unlock();
                return result;
            }
        }
        //实例更新
        m_books.push_back(std::move(infoCombineBook(std::move(book) ) ));
        m_locker.unlock();
        return result; //成功
    };
    int deleteBook(const string & bookId)
    {
        m_locker.lock();
        list<infoCombineBook>::iterator it ,end ;
        it = m_books.begin();
        end = m_books.end();
        int result = 1 ;//默认删除成功
        for(;it != end;it++){
            if(it->getBookId() == bookId){
                m_books.erase(it);
                m_locker.unlock();
                return result;
            }
        }
        m_locker.unlock();
        result = -1 ;
        return result ;//查找不到返回
    };
    int updateBook(const BookInfoTable & book)
    {
        return 0 ;
    }
    //模糊匹配对应本数,对应起点的书籍
    int  fuzzySearch(vector<BookInfoTable> & books ,const string & words ,
            const int & offset,const int & count)
    {
        //参数检查
        if(words == "" || offset < 0 || count <= 0){
            return -1;
        }
        cout<<" 进入 "<<endl ;
        m_locker.lock();
        map<int,infoCombineBook> buffer ;//匹配结果
        list<infoCombineBook>::iterator it ,end ;
        it = m_books.begin();
        end = m_books.end();
        int score ;
        cout<<"  internal size is "<<m_books.size()<<endl;
        for(;it != end;it++){
            score = (int)(10000 * rapidfuzz::fuzz::ratio(it->getCombineInfo(),words) );
            cout<<"  info   and  score "<<it->getCombineInfo()<<" "<<score<<endl;
            if(score != 0)
                buffer.insert(pair<int,infoCombineBook>(score,*it));
        }
        map<int,infoCombineBook>::iterator bufBgein,bufEnd ;
        bufBgein = buffer.begin();
        bufEnd = buffer.end();
        int number = 0 ;
        do{
            bufBgein++ ;
            number++ ;
        }while(number == offset );
        //移动后添加
        BookInfoTable book ;
        for(int index = 0 ;index < count && bufBgein != bufEnd ;index++,bufBgein++ ){
            
            bufBgein->second.getTranslateBook(book);
            books.push_back(book);
        }
        m_locker.unlock();
        return 1;
    };
    int  size(){
        return (int)m_books.size();
    }
private:
    locker m_locker; //加锁对于实例化书籍进行保护
    list<infoCombineBook>m_books;
};


#endif
