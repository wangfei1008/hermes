
#include "hermes.h"
#include "log/log.h"
#include "service.h"

using namespace std;

int main()
{
	LOG_INIT();
	LOGINFO("****************************** setup ****************************");
	Service s;
	s.init();
	// 阻塞主线程，等待输入 'q' 退出
	char c = 0;
	while (c = getc(stdin)) {
		if ('EOF' == c || 'q' == c || 'Q' == c) break;
	}
	s.release();
	LOGINFO("****************************** stop *****************************");
	LOG_SHUTDOWN();
	return 0;
}
