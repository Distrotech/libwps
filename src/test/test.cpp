/* libwps
 * Copyright (C) 2006 Andrew Ziem
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#include <iostream>
#include <stdio.h>
#include <unistd.h>

#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/CompilerOutputter.h>

#include "libwps_internal.h"
#include "WPSStreamImplementation.h"


#define TMP_FILENAME "libwps-unittest.tmp"

class Test : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE(Test);
  CPPUNIT_TEST(testStream);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void);
  void tearDown(void);

protected:
  void testStream(void);

};

void Test::setUp(void)
{
	FILE *f = fopen(TMP_FILENAME, "w");
	fprintf(f, "%c%c%c%c",1,2,3,4);
	fprintf(f, "%c%c%c%c",0,5,6,7);
	fclose(f);
}

void Test::tearDown(void)
{
	unlink(TMP_FILENAME);
}

void Test::testStream(void)
{ 
	WPSInputStream* input = new WPSFileStream(TMP_FILENAME);


	// fixme: should isOle() move the cursor?
	CPPUNIT_ASSERT_EQUAL ( false, input->isOle() );
	CPPUNIT_ASSERT_EQUAL ( (WPSInputStream*) NULL, input->getWPSOleStream("foo") );

	// test read()
	char buffer[100];
	CPPUNIT_ASSERT_EQUAL ( (long int) 0 , input->read(0, NULL) );
#if 1
	CPPUNIT_ASSERT_EQUAL ( (long int) 0 , input->read(1, NULL) );
	CPPUNIT_ASSERT_EQUAL ( (long int) 0 , input->read(-1, NULL) );
#endif
	input->seek(0);
	CPPUNIT_ASSERT_EQUAL ( (long int) 0 , input->read(0, buffer) );
	CPPUNIT_ASSERT_EQUAL ( (long int) 0 , input->tell() );
	CPPUNIT_ASSERT_EQUAL ( (long int) 1 , input->read(1, buffer) );
	CPPUNIT_ASSERT_EQUAL ( (long int) 1 , input->tell() );
#if 1
	input->seek(0);
	CPPUNIT_ASSERT_EQUAL ( (long int) 8 , input->read(50, buffer) );
#endif

	// test readU*()
	input->seek(0);
	CPPUNIT_ASSERT_EQUAL( (uint8_t) 1 , readU8(input) );
	CPPUNIT_ASSERT_EQUAL( (uint8_t) 2 , readU8(input) );
	CPPUNIT_ASSERT_EQUAL( (uint8_t) 3 , readU8(input) );
	CPPUNIT_ASSERT_EQUAL( (uint8_t) 4 , readU8(input) );

	input->seek(0);
	CPPUNIT_ASSERT_EQUAL( (uint16_t) 0x0201 , readU16(input) );
	CPPUNIT_ASSERT_EQUAL( (uint16_t) 0x0403 , readU16(input) );

	input->seek(0);
	uint32_t u32 = readU32(input);
	CPPUNIT_ASSERT_EQUAL( (uint32_t) 0x04030201 , u32 );
	u32 = readU32(input);
	CPPUNIT_ASSERT_EQUAL( (uint32_t) 0x07060500 , u32 );

	// test seek(), tell(), atEnd()
	input->seek(1);
	CPPUNIT_ASSERT_EQUAL ( (long int) 1 , input->tell() );

	input->seek(0);
	CPPUNIT_ASSERT_EQUAL ( (long int) 0 , input->tell() );

	input->seek(8);
	CPPUNIT_ASSERT_EQUAL ( (long int) 8 , input->tell() );

	input->seek(0);
	//fixme: should 9 be 8?
	for (int i = 0; i < 9; i++)
		readU8(input);
	CPPUNIT_ASSERT_EQUAL ( true, input->atEnd() );

#if 1
	input->seek(8);
	CPPUNIT_ASSERT_EQUAL ( true, input->atEnd() );

	input->seek(-1);
	CPPUNIT_ASSERT_EQUAL ( (long int) 0, input->tell() );

	input->seek(10000);
	CPPUNIT_ASSERT( 10000 != input->tell() );
	CPPUNIT_ASSERT( input->atEnd() );
#endif

	delete input;
}

CPPUNIT_TEST_SUITE_REGISTRATION(Test);

int main( int argc, char **argv )
{
	// Create the event manager and test controller
	CPPUNIT_NS::TestResult controller;

	// Add a listener that colllects test result
	CPPUNIT_NS::TestResultCollector result;
	controller.addListener( &result );        

	// Add a listener that print dots as test run.
	CPPUNIT_NS::BriefTestProgressListener progress;
	controller.addListener( &progress );      

	// Add the top suite to the test runner
	CPPUNIT_NS::TestRunner runner;
	runner.addTest( CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest() );
	runner.run( controller );

	// output
	CPPUNIT_NS::CompilerOutputter outputter( &result, std::cerr );
	outputter.write();

	// return status code
	return result.wasSuccessful() ? 0 : 1;
}
