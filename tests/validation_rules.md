## Basic validation rules

* **hash** length >7

## Curly braces with operators are allowed only for:
	via-port
	with-interface
	if

## Curly braces without operators are allowed only for
	with-interface

## List of conditions:
* **localtime(time_range)**: evaluates to true if the local time is in the specified time range. time_range can be written either as HH:MM[:SS] or HH:MM[:SS]-HH:MM[:SS].
* **allowed-matches(query)**: evaluates to true if an allowed device matches the specified query. The query uses the rule syntax. Conditions in the query are not evaluated.
* **rule-applied**: evaluates to true if the rule currently being evaluated ever matches a device.
* **rule-applied(past_duration)**: evaluates to true if the rule currently being evaluated matched a device in the past duration of time specified by the parameter. past_duration can be written as HH:MM:SS, HH:MM, or SS.
* **rule-evaluated**: evaluates to true if the rule currently being evaluated was ever evaluated before.
* **rule-evaluated(past_duration)**: Evaluates to true if the rule currently being evaluated was evaluated in the past duration of time specified by the parameter. past_duration can be written as HH:MM:SS, HH:MM, or SS.
* **random**: Evaluates to true/false with a probability of p=0.5.
* **random(p_true)**: Evaluates to true with the specified probability p_true.
* **true**: Evaluates always to true.
* **false**: Evaluates always to false.

## Interpretation of the set operator:

* **all-of**: Evaluate to true if all of the specified conditions are true.
* **one-of**: Evaluate to true if one of the specified conditions is true.
* **none-of**: Evaluate to true if none of the specified conditions is true.
* **equals**: Same as all-of.
* **equals-ordered**: Same as all-of
	
## Sorting rules:
* **port**, condition fields are not used for hash-evaluating, so if one of the rules contains some of these values, 
It will be sorted as a row rule.
	