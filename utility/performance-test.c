// Usage in Pike:   time pike -D_test_new_objects performance-test.c
// Usage in LPC:    cp performance-test.c ../world/test.c
//		    time psyclpc -E 0 -f "test main" -D_test_new_objects
//
// Measurements aren't too accurate as they include compilation time.

/*
 * This file was written to find out which data container types perform
 * best: arrays, mappings or objects. Then I decided to also look at
 * pike's behaviour on this topic.
 *
 * Results:

LPC:	even with psyclpc's new streamlined object code, using
objects takes double as much computation time than using mappings
or arrays. this presumes you will be using objects where each
attribute has its own variable - but it is in fact more likely
that you will be using mappings with an object class wrapper, so
the factor rises to 3. doing traditional OO programming is a bit
expensive in LPC, it seems. but probably it's not different in
other languages, it's just a tradeoff which is generally accepted
for the benefit of having OO with encapsulation and everything.

PIKE:	as expected the optimizations generally make pike double
as fast as LPC, however there is an interesting detail with
arrays being 3 times as fast as objects. so even in pike it is
recommendable to use OO only where it is really worth it. note
also the result for _test_new_objects must be wrong, how can
cloning 300000 MyTests come out cheaper than just using one (as
shown in _test_using_objects?). I presume some compile-time
optimization killed the intended purpose of the test, so we have
no usable estimate on object cloning.	:(

 *
 * This is hybrid pike/LPC code:
 */

int loops = 300000;

#ifdef LPC3
# include <net.h>
# include <sys/debug_message.h>
#endif

#ifdef __PIKE__
class MyTest {
#endif
	// TEST OBJECT CODE START
	mixed p;

	mixed attri(mixed put) {
		if (put) p = put;
		return p;
	}

	int one() {
		return 1;
	}
	// TEST OBJECT CODE STOP
#ifdef __PIKE__
}
		// minor advantage for pike.. we are not cloning main()
#endif

mixed main(mixed x, mixed y) {
	int i, sum = 0;
	mixed t;
	array(int) a = ({ 1, 2 });
	mapping m = ([ "one": 1, "two": 2 ]);
	object o;

	//PP(("\nx=%O y=%O\n\n", x, y));
	//
#ifdef LPC3
	// apparently stderr gets flushed anyway, so i can make nice
	// nice incremental outputs there.. ;)
	debug_message("Running tests ..", DMSG_STDERR);
//	printf("... %c ...", "ufo"[-1]);    // LPC requires <1 here
	o = clone_object(ME);
#else
	write("Running tests ..");
	o = MyTest();
#endif
					// results using loops = 300000
	for (i=loops; i; i--) {
#ifdef _test_new_objects		// LPC: 1.900u 0.040s 0:01.94 100.0%
# ifdef __PIKE__			//pike: 0.468u 0.004s 0:00.46 100.0%
		o = MyTest();		//   pike result must be wrong  :( 
# else
		/* also has significantly higher eval_cost */
		o = clone_object(ME);
# endif
		sum += o->one();
		destruct(o);
#endif
#ifdef _test_new_mappings		// LPC: 0.704u 0.016s 0:00.72 98.6%
		m = ([ ]);		//pike: 0.336u 0.024s 0:00.35 100.0%
		m["one"] = 1;
		sum += m["one"];
#endif
#ifdef _test_new_arrays			// LPC: 0.572u 0.012s 0:00.59 98.3%
		a = ({ });		//pike: 0.292u 0.000s 0:00.29 100.0%
		a += ({ 1 });
		sum += a[0];
#endif
#ifdef _test_using_objects		// LPC: 1.164u 0.008s 0:01.17 99.1%
		o->attri(4404);		//pike: 0.488u 0.004s 0:00.49 97.9%
		t = o->attri();
		sum += o->one();
#endif
#ifdef _test_using_mappings		// LPC: 0.588u 0.032s 0:00.62 98.3%
		m["4404"] = 4404;	//pike: 0.292u 0.004s 0:00.29 100.0%
		t = m["4404"];
		sum += m["one"];
#endif
#ifdef _test_using_arrays		// LPC: 0.508u 0.012s 0:00.52 98.0%
		a[1] = 4404;		//pike: 0.188u 0.004s 0:00.19 94.7%
		t = a[1];
		sum += a[0];
#endif
	}
	t = "Result is "+ ( sum == loops? "correct": "wrong" ) +".\n";
#ifdef LPC3
	debug_message(". done!\n"+ t, DMSG_STDERR);
	return "sum: "+ sum;
#else
	write(". done!\n"+ t);
	return 0;
#endif
}

