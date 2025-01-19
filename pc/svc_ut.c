////////////////////////////////////////////////////////////////////////////////
//
// Utility Services
//
////////////////////////////////////////////////////////////////////////////////


// UT$SPLT
//
// Split a field from a buffer, using a field list to find the field
// The field list is in the QCode format (it is a pointer to the QCode

// All data is binary, in case we support binary types
// Field is identified by indexinto the field list
//

int ut_splt(uint8_t *field_list, int field_list_n, uint8_t **field, int *field_n)
{
  int fld_num;
  
  // Get field number from name
  fld_num = ut_find_field(field_list, field_list_n, *field, *field_n);

  if( fld_num == -1 )
    {
      // Bad field name
    }

  
}
