/*******************************************************************************
################################################################################
#   Copyright (c) [2017-2019] [Radisys]                                        #
#                                                                              #
#   Licensed under the Apache License, Version 2.0 (the "License");            #
#   you may not use this file except in compliance with the License.           #
#   You may obtain a copy of the License at                                    #
#                                                                              #
#       http://www.apache.org/licenses/LICENSE-2.0                             #
#                                                                              #
#   Unless required by applicable law or agreed to in writing, software        #
#   distributed under the License is distributed on an "AS IS" BASIS,          #
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   #
#   See the License for the specific language governing permissions and        #
#   limitations under the License.                                             #
################################################################################
 *******************************************************************************/
/* header include files (.h) */
#include "common_def.h"
#include "mac_utils.h"


uint32_t shortBsrBytesTable[MAX_SHORT_BSR_TABLE_ENTRIES] = { 0, 10, 14, 20, 28, \
   38, 53, 74, 102, 142, 198, 276, 384, 535, 745, 1038, 1446, 2014, 2806, 3909, \
   5446, 7587, 10570, 14726, 20516, 28581, 39818, 55474, 77284, 107669, 150000, \
   150001 };
