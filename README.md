
Attribute Debugging Module for FreeRADIUS
=========================================

Attribute Debugging Module for FreeRADIUS  
Copyright (C) 2026 David M. Syzdek <david@syzdek.net>.  
All rights reserved.  


Overview
--------

The Attribute Debugging Module for FreeRADIUS (rlm_show_attrs) displays the
prints debugging entries containing the current values of the attributes.

rlm_show_attrs has been tested with FreeRADIUS server 3.2.x.


Module Configuration
--------------------

   * ___control_attributes___ - include control attributes in debugging
     output. The default is "_true_".

   * ___reply_attributes___ - include reply attributes in debugging output.
     The default is "_true_".

   * ___request_attributes___ - include request attributes in debugging
     output. The default is "_true_".

   * ___date_strings__ - display date attributes as a string instead of as a
     number representing the seconds since Janyary 1, 1970.  The default is
     "_false_".

   * ___use_gmtime___ - display _DATE_ attributes as GM time instead of
     as local time.  The default is "_false_".


The following is a example configuration which uses the default values:

      show_attrs {
         control_attributes   = true
         reply_attributes     = true
         request_attributes   = true
         date_strings         = false
         use_gmtime           = false
      }


Module Usage
------------

The following is a simple example of a server using the show_attrs module:

      server show-attrs {
         authorize {
            # obtain the users' TOTP secret from a data store.  If the secret
            # is base32 encoded, set the secret to "&control:TOTP-Secret". If
            # the secret is binary, set the secret to "&control:TOTP-Key".
            -ldap
            -sql
            
            show_attrs
            
            pap
            chap
            mschap
         }
         authenticate {
            Auth-Type PAP {
               pap
            }
            Auth-Type CHAP {
                chap
            }
            Auth-Type MS-CHAP {
               mschap
            }
            show_attrs
         }
         post-auth {
            show_attrs
            Post-Auth-Type REJECT {
               show_attrs
            }
         }
      }


Installing Module
-----------------

The rlm_show_attrs module must be compiled when the FreeRADIUS server is
compiled.  First download the source code for the FreeRADIUS server and 
rlm_show_attrs.

Unpack the source code:

    tar -xf freeradius-server-x.x.x.tar.gz
    tar -xf rlm_show_attrs-x.x.tar.gz
    make -C rlm_show_attrs-x.x FREERADIUS_SOURCE=../freeradius-server-x.x.x prepare

Follow the instructions for building FreeRADIUS server from source (see below
for links).


References
----------

   * [FreeRADIUS Documentation](https://wiki.freeradius.org):
     - [FreeRADIUS Modules](https://www.freeradius.org/modules/)
     - [FreeRADIUS Development](https://www.freeradius.org/documentation/freeradius-server/current/developers/)
     - [Creating modules for FreeRADIUS v3.0.x](https://wiki.freeradius.org/contributing/Modules3)
     - [Documentation Guidelines](https://www.freeradius.org/documentation/freeradius-server/4.0.0/developers/guidelines.html)

   * Compiling FreeRADIUS
     - [Building from Source](https://www.freeradius.org/documentation/freeradius-server/current/installation/source.html)
     - [Building FreeRADIUS](https://wiki.freeradius.org/building/Home)

   * Source Code:
     - [FreeRADIUS Server](https://github.com/FreeRADIUS/freeradius-server)
     - [FreeRADIUS 3.2.x rlm_example](https://github.com/FreeRADIUS/freeradius-server/tree/v3.2.x/src/modules/rlm_example)
     - [Attribute Debugging Module for FreeRADIUS](https://github.com/syzdek/rlm_show_attrs)

