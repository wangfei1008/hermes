#ifndef __NOCOPYABLE_H__
#define __NOCOPYABLE_H__

//继承了本类的成员都将具有对象语义
//即无法实现对象赋值和对象复制的操作
class NoCopyable{
	protected:
		NoCopyable(){

		}

		~NoCopyable(){

		}

		NoCopyable(const NoCopyable& rhs) = delete;
		NoCopyable& operator=(const NoCopyable& rhs) = delete;
};

#endif