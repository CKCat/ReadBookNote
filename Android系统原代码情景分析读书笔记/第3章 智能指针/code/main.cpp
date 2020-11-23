#include <stdio.h>
#include "RefBase.h"

using namespace android;

class LightClass : public LightRefBase<LightClass>
{
public:
	LightClass() {
		printf("Construct LightClass Object.");
	}

	~LightClass() {
		printf("Destroy LightClass Object.");
	}
};

int main(int argc, char* argv[]) {
	LightClass* plightclass = new LightClass();
	sp<LightClass> lpOut = plightclass;
	printf("Light Ref Count: %d.\n", plightclass->getStrongCount());
	{
		sp<LightClass> lpInner = lpOut;
		printf("Light Ref Count: %d.\n", plightclass->getStrongCount());

	}
	printf("Light Ref Count: %d.\n", plightclass->getStrongCount());

	return 0;
}