var casper = require("casper").create({
	onPageInitialize: function() {
		casper.evaluate(function(){
			swfobject = {};
			swfobject.getFlashPlayerVersion = function() {return {major: 10, minor: 0};};
			swfobject.hasFlashPlayerVersion = function(version) {return version == "10"};
			Modernizr.audio.m4a = true;
			Modernizr.audio.mp3 = true;
		});
	}
});
var url = "http://www.pandora.com/account/sign-in";

casper.start(url);


casper.then(function() {
	var counter = 0,
	    _this = this;

	var f = function(){
		if (_this.exists('form.loginForm')) {
	        _this.echo('found #my_super_id');
	        return;
	    } else {
	        _this.echo('#my_super_id not found');
	    }
	    counter++;
	    if(counter>5){
	    	_this.echo('timed out');
	    }
	    setTimeout(f, 1000);

	};

	f();

    /*casper.waitForSelector('form.loginForm', function() {
	    this.echo("Found login form");
	    this.fill('form.loginForm', {
	        'email':    'pennapps@team.com',
	        'password':    'password',
	    }, true);
	}, function() { this.echo("Timeout... :("); }, 10000);*/
});

casper.then(function() {
	//this.debugHTML();
    //this.wait(2000);
});

casper.then(function() {
    //this.echo(this.getCurrentUrl());
});

casper.run();