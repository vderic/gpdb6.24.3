// Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
#ifndef NODEAGG_I_H
#define NODEAGG_I_H

typedef struct AggStatePerAggData exx_agg_state_per_agg_t;
typedef struct AggStatePerGroupData exx_agg_pergroup_t;

extern bool agg_get_agg_state_per_agg(struct AggState* aggstate, 
        int aggno,
        exx_agg_state_per_agg_t* data);

#endif /* NODEAGG_I_H */

