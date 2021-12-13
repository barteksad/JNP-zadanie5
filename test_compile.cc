#include "virus_genealogy.h"
#include <vector>
#include <iostream>
using namespace std;

class Virus
{
public:
	using id_type = int;

	Virus(Virus::id_type _id)
		: id(_id)
	{
	}

	Virus::id_type get_id() const
	{
		return id;
	}

private:
	id_type id;
};

int main()
{
	VirusGenealogy<Virus> g(0);
	try {

		vector<int> a({0});
		vector<int> b({1});
		vector<int> c({0, 1});
		vector<int> d({2});
		vector<int> e({0, 1, 2, 3, 4});
		vector<int> f({0, 1, 2, 3, 4, 5});
		vector<int> h({0, 1, 2, 3, 4, 5, 6});
		vector<int> i({0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
		g.create(1, a);
		g.create(2, b);
		g.create(3, c);
		g.create(4, d);
		g.create(5, e);
		g.create(6, f);
		g.create(7, h);
		g.create(8, 1);
		g.create(9, 8);
		g.create(10, i);
	} catch (...) {
		;
	}

	g.print();

	cout << "end\n";
}