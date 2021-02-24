// @Autthor: Lin Huang

#ifndef __UTILS_UNCOPYABLE_HEAD
#define __UTILS_UNCOPYABLE_HEAD

class uncopyable{
public:
	uncopyable() {}
	~uncopyable() {}
	uncopyable(const uncopyable&)=delete;
	uncopyable& operator=(const uncopyable&)=delete;
};


#endif
