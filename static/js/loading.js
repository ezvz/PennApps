$(document).ready(function(){
	$('#submit').click(function() {
		$('.modal2').show();
    });

	$("#pandoraUsername").validate({
	  expression: "if(VAL != '') return true; else return false;",
	  message: "Valid Pandora Username is required."
	});

	$("#pandoraPassword").validate({
	  expression: "if(VAL != '') return true; else return false;",
	  message: "Password is required."
	});

	$("#spotifyUsername").validate({
	  expression: "if(VAL != '') return true; else return false;",
	  message: "Spotify Username is required."
	});

	$("#spotifyPassword").validate({
	  expression: "if(VAL!= '') return true; else return false;",
	  message: "Spotify password is required."
	});
    
});