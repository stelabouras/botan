Botan 2.x to 3.x Migration
==============================

This is a guide on migrating applications from Botan 2.x to 3.0.  If you find
that some functionality you need has been removed in 3.0 please open an issue on
Github - the goal is to remove only APIs where have a superior alternative
exists or where the API is just hopelessly broken.

Headers
--------

Many headers have been removed from the public API.

In some cases, such as ``datastor.h`` or ``tls_blocking.h``, the functionality
presented was entirely deprecated, in which case it has been removed.

In other cases (such as ``loadstor.h`` or ``rotate.h``) the header was really an
implementation header of the library and not intended to be consumed as a public
API. In these cases the header is still used internally, but not installed for
application use.

However in most cases there is a better way of performing the same operations,
which usually works in both 2.x and 3.x. For example, in 3.0 all of the
algorithm headers (such as ``aes.h``) have been removed. Instead you should
create objects via the factory methods (in the case of AES,
``BlockCipher::create``) which works in both 2.x and 3.0

Algorithms Removed
-------------------

The algorithms CAST-256, MISTY1, Kasumi, DESX, XTEA, PBKDF1, MCEIES, CBC-MAC and
Tiger have been removed. The expectation is that literally nobody was using any
of these algorithms for anything. All are obscure, and many are (more or less)
broken.

Exception Changes
-------------------

Several exceptions, mostly ones not used by the library, were removed.

A few others that were very specific (such as Illegal_Point) were replaced
by throws of their immediate base class exception type.

The base class of Encoding_Error and Decoding_Error changed from
Invalid_Argument to Exception. If you are explicitly catching Invalid_Argument,
verify that you do not need to now also explicitly catch Encoding_Error and/or
Decoding_Error.

X.509 Certificate Info Access
-------------------------------

Previously ``X509_Certificate::subject_info`` and ``issuer_info`` could be used
to query information about extensions. This is not longer the case; instead you
should either call a specific function on ``X509_Certificate`` which returns the
same information, or lacking that, iterate over the result of
``X509_Certificate::v3_extensions``.

Use of ``enum class``
--------------------------------

Several enumerations where modified to become ``enum class``, including
``DL_Group::Format``, ``CRL_Code``, ``EC_Group_Encoding``,

