/*
 * Copyright (c) 2021 DIY356
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MGOS_BFLOWSENS_GPIO_H_
#define MGOS_BFLOWSENS_GPIO_H_

#include <stdbool.h>
#include "mgos_bflowsens.h"
#include "mgos_bvar_dic.h"

#ifdef __cplusplus
extern "C" {
#endif

bool mgos_bflowsens_gpio_attach(mgos_bflowsens_t sensor, 
                                  int pin, enum mgos_gpio_pull_type pull_type,
                                  bool high_pulse, float flow_ratio);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_BFLOWSENS_GPIO_H_ */