/* autogarten : https://github.com/lbracken/autogarten
   :license: MIT, see LICENSE for more details.
*/



function renderSparklines() {
	/*
	 * Based upon D3 sparkline example at:
	 *   http://bl.ocks.org/benjchristensen/1133472
	 * 
	 * Quick numbers for reference
	 *   96 data points = 24 hrs  @ 15min gran
	 *  168 data points = 07 days @ 1hr gran
	 *  180 data points = 30 days @ 4hr gran
	 */

	// Get all sparkline containers and compute width/height from first sparkline
    var sparklines = document.querySelectorAll(".sparkline");
   	var sparklineWidth = (sparklines.length > 0) ? sparklines[0].offsetWidth : 0;
	var sparklineHeight = (sparklines.length > 0) ? sparklines[0].offsetHeight : 0;

	// Iterator over each sparkline in the page
    for (i=0; i < sparklines.length; i++) {

    	// Create an SVG element for the sparkline
    	var sparkline = d3.select(sparklines[i])
    		.append("svg:svg")
			.attr("width", "100%")
			.attr("height", "100%");

		// Read data for this sparkline
		var data = $.parseJSON(sparklines[i].getAttribute("data"));

		// Create scales for rendering data. For more info on scales...
		//  * https://github.com/mbostock/d3/wiki/Quantitative-Scales
		//  * http://chimera.labs.oreilly.com/books/1230000000345/ch07.html
		var xScale = d3.scale.linear()
			.domain([0, data.values.length])
			.range([0, sparklineWidth]);

		var yScale = d3.scale.linear()
			.domain([data.min_value, data.max_value])
			.range([sparklineHeight, 0]);
		
		// Create an SVG line as the sparkline
		var line = d3.svg.line()
			.x(function(d,i) { return xScale(i); })
			.y(function(d) { return yScale(d); });
	
		sparkline.append("svg:path").attr("d", line(data.values));
    }
}


$(document).ready(function() {
	renderSparklines();
});