#include "hermes.h"
#include "log/log.h"
#include "service.h"
#include <cstdio>

int main()
{
	LOG_INIT("./log.properties");
	LOGINFO("****************************** setup ****************************");
	Service s;
	s.init();
	// 阻塞主线程，等待输入 'q' 退出
	int c;
	while ((c = getc(stdin)) != EOF) {
		if (c == 'q' || c == 'Q') break;
	}
	s.release();
	LOGINFO("****************************** stop *****************************");
	LOG_SHUTDOWN();
	return 0;
}
