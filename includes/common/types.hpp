#ifndef TICKET_SYSTEM_TYPES_HPP
#define TICKET_SYSTEM_TYPES_HPP

#include "common/fixed_string.hpp"

using Username = FixedString<21>;
using Password = FixedString<31>;
using Name = FixedString<16>;
using MailAddr = FixedString<31>;
using Privilege = char;

using TrainID = FixedString<21>;
using Station = FixedString<31>;

#endif  // TICKET_SYSTEM_TYPES_HPP
