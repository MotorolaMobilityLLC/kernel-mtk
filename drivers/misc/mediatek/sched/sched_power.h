
/* turning_point:
 *	to help identify if "more-core at low freq" is suffered
 *	in share-bucked when the high OPP is triggered by heavy task.
 *  watershed:
 *	to quantify what ultra-low loading is "less-cores" prefer.
*/

#define DEFAULT_TURNINGPOINT 65
#define DEFAULT_WATERSHED 236

struct power_tuning_t {
	int turning_point; /* max=100, default: 65% capacity */
	int watershed; /* max=1023 */
};


extern struct power_tuning_t *get_eas_power_setting(void);
