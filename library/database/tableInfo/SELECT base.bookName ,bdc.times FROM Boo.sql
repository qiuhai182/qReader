SELECT base.bookName ,bdc.times FROM BookBaseInfoTable base 
LEFT JOIN 
BookDownloadCountTable bdc
 ON base.autoBookId = bdc.autoBookId ;
 AND 
 base.bookName 
LIKE 'Linux' AND base.isDelete = 0 
ORDER BY 
bdc.times 
DESC LIMIT 0  ,  4 ;

select bookName  from BookBaseInfoTable 
where bookType = '计算机与互联网' AND isDelete = 0 and autoBookId IN (SELECT base.autoBookId FROM BookBaseInfoTable base 
LEFT JOIN 
BookDownloadCountTable bdc
 ON base.autoBookId = bdc.autoBookId order by bdc.times desc );


SELECT base.bookName ,bdc.times FROM BookBaseInfoTable base 
LEFT JOIN 
BookDownloadCountTable bdc
ON base.autoBookId = bdc.autoBookId and bookType = '计算机与互联网' AND isDelete = 0 order by bdc.times desc ;


(SELECT * from BookBaseInfoTable subBase
where  bookType = '计算机与互联网' AND isDelete = 0) 
left JOIN
BookDownloadCountTable bdc
; 

