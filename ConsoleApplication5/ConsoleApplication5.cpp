#include "pch.h"
#include <iostream>
#include "windows.h"
using namespace std;

/*Virtual Method Table Hooking:
A virtual method table is created for every class that creates or inherits any virtual functions. A virtual method table holds an array of pointers to each
virtual method that instances of the class are able to call. Furthermore, each instance of a class with virtual functions obtains a hidden member variable 
which points to the virtual method table for that class. This pointer is stored in the first 4 bytes of every instance of the class.

Virtual method table hooking allows us to change the functions that are pointed to within a virtual method table. We can redirect pointers within a virtual
method table to our own functions to execute our own code. Example:*/


class VirtualClass
{
public:

	int number;

	virtual void VirtualFn1(void) /*Because this class has at least one virtual function, a vtable is created for it. The vtable is an array that holds the
				      address of each virtual function that the class can access. Also, a hidden pointer variable is added to each instance of the
				      class, which points to the vtable. The vtable pointer is stored at the first 4 bytes of each class instance.
								  
				      Note that every time a class member function is called, it is called with the __thiscall calling convention (unless the
				      member function has a variable number of arguments in which case it's called with __cdecl, but that is not the case here). 
				      The __thiscall convention passes the "this" pointer of the calling object into the ECX register. This is important to take
				      into consideration when calling our hook function...*/
	{
		cout << "VirtualFn1 called " << number++ << "\n\n";
	}
};





using VirtualFn1_t = void(__thiscall*)(void* thisptr); /*Creates a type called VirtualFn1_t. This type holds a pointer to a function which returns void and 
				                       takes a void* parameter (which is the "this" pointer to a class instance). This is a template for a 
						       function of the same type as our virtual function (VirtualFn1) that we wish to hook.

						       Alternative typedef syntax:    typedef void(__thiscall* VirtualFn1_t)(void* thisptr);*/

VirtualFn1_t orig_VirtualFn1; /*Creates a variable (orig_VirtualFn1) of type VirtualFn1_t. This will be used later to store the address of the original
	                      VirtualFn1 function before we hook it, allowing us to keep accessing it after the hook.*/





void __fastcall hkVirtualFn1(void* thisptr, int edx) /*This is our custom hook function. It will be called instead of the original VirtualFn1 function after
						     hooking it. Here we use the __fastcall calling convention which requires at least two parameters - the 
						     first two arguments are passed in via the ECX and EDX registers, and any other additional arguments are
						     passed in via the stack. Here the first parameter is the "this" pointer to the class instance and the 
						     second parameter is a placeholder that is not used for anything (except to satisfy the __fastcall 
						     requirement of having at least two parameters).
													 
						     When VirtualClass::VirtualFn1() is called, it is called with the __thiscall calling convention (as
						     previously explained). This passes the "this" pointer of the class instance into ECX. Then when our hook
						     function is called immediately afterwards, it needs to look into ECX to find the "this" pointer
						     argument. As a result, we use the __fastcall calling convention, because __fastcall looks into ECX
						     for its first argument. Other calling conventions like __cdecl and __stdcall only look onto the stack
						     for arguments, which would cause us to lose track of the "this" pointer.

						     Note that our hook function cannot just use __thiscall itself because it is not a class member function, 
						     so __fastcall has to be used instead (it's the only other calling convention that looks into ECX for an
						     argument so it's the only one that can intercept the "this" argument correctly).*/
{
	cout << "Custom function called" << "\n";

	orig_VirtualFn1(thisptr); //Calls the original function after our own is done.
}





int main()
{
	VirtualClass* myClass = new VirtualClass(); //Create a pointer to a VirtualMethod instance called myClass.

	void** vTablePtr = *(void***)myClass; /*myClass is cast to a pointer to a pointer to a pointer (pointer to the base class, then pointer to the vtable 
					      base, then pointer to a virtual function). myClass is then dereferenced one level and the result is stored in 
					      vTablePtr. Therefore vTablePtr is a pointer to the vtable which then points to a virtual function. Note that 
					      reinterpret_cast can also be used instead of a C-style cast.*/

	DWORD oldProtection; //Buffer that will hold the previous page protection level. Required to call VirtualProtect.

	VirtualProtect(vTablePtr, 4, PAGE_EXECUTE_READWRITE, &oldProtection); //Remove page protection at location to overwrite.

	orig_VirtualFn1 = (VirtualFn1_t)*vTablePtr; /*Dereference vTablePtr one level to find the address of the first function in the vtable (which is the 
						    address of VirtualFn1). Then cast this to a VirtualFn1_t and store it in orig_VirtualFn1.*/

	*vTablePtr = &hkVirtualFn1; /*Dereference vTablePtr (which is the vtable pointer) to find the address of the first function in the vtable (VirtualFn1). 
				    Then replace it with the address of our hook function. This means that every time the program tries to call VirtualFn1, it 
				    will actually call our hkVirtualFn1 instead.*/

	VirtualProtect(&vTablePtr[0], 4, oldProtection, 0); /*Restore page protection. Note that "&vTablePtr[0]" is pointer arithmetic that takes the address of
							    vTablePtr, adds 0 to it, then dereferences. This simply brings us back to the original vTablePtr but 
							    is here simply to demonstrate pointer arithmetic with the subscript operator.*/

	myClass->VirtualFn1(); //Call the virtual function (now hooked) from our class instance.
	myClass->VirtualFn1();
	myClass->VirtualFn1();

	delete myClass;

	return 0;
}
