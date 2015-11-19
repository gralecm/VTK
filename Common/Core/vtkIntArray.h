/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIntArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIntArray - dynamic, self-adjusting array of int
// .SECTION Description
// vtkIntArray is an array of values of type int.  It provides
// methods for insertion and retrieval of values and will
// automatically resize itself to hold new data.
//
// The C++ standard does not define the exact size of the int type, so use
// of this type directly is discouraged.  If an array of 32 bit integers is
// needed, prefer vtkTypeInt32Array to this class.

#ifndef vtkIntArray_h
#define vtkIntArray_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkDataArray.h"
#include "vtkAoSDataArrayTemplate.h" // Real Superclass

// Fake the superclass for the wrappers.
#ifndef __WRAP__
#define vtkDataArray vtkAoSDataArrayTemplate<int>
#endif
class VTKCOMMONCORE_EXPORT vtkIntArray : public vtkDataArray
{
public:
  vtkTypeMacro(vtkIntArray, vtkDataArray)
#ifndef __WRAP__
#undef vtkDataArray
#endif
  static vtkIntArray* New();
  void PrintSelf(ostream& os, vtkIndent indent);

  // This macro expands to the set of method declarations that
  // make up the interface of vtkAoSDataArrayTemplate, which is ignored
  // by the wrappers.
#if defined(__WRAP__) || defined (__WRAP_GCCXML__)
  vtkCreateWrappedArrayInterface(int);
#endif

  // Description:
  // A faster alternative to SafeDownCast for downcasting vtkAbstractArrays.
  static vtkIntArray* FastDownCast(vtkAbstractArray *source)
  {
    return static_cast<vtkIntArray*>(Superclass::FastDownCast(source));
  }

  // Description:
  // Get the minimum data value in its native type.
  static int GetDataTypeValueMin() { return VTK_INT_MIN; }

  // Description:
  // Get the maximum data value in its native type.
  static int GetDataTypeValueMax() { return VTK_INT_MAX; }

protected:
  vtkIntArray();
  ~vtkIntArray();

private:
  //BTX
  typedef vtkAoSDataArrayTemplate<int> RealSuperclass;
  //ETX
  vtkIntArray(const vtkIntArray&);  // Not implemented.
  void operator=(const vtkIntArray&);  // Not implemented.
};

// Define vtkArrayDownCast implementation:
vtkArrayDownCast_FastCastMacro(vtkIntArray)

#endif
