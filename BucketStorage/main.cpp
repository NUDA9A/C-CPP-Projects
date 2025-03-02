//
// Created by vlaki on 13.06.2024.
//

#include "helpers.h"

int main()
{
	BucketStorage< NoCopy > a = BucketStorage< NoCopy >();

	a.insert(NoCopy(2));
	a.insert(NoCopy(1));

	Iterator< NoCopy > it = std::find(a.begin(), a.end(), NoCopy(1));

	if (it == a.begin())
	{
		puts("yes");
	}


	return 0;
}
