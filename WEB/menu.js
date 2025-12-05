function Menu()
{
	var Pages = document.getElementsByTagName("DIV");
	var Elements = []
	for (i in Pages)
	{
		var Needle = new RegExp("(^|\\s+)MenuPage($|\\s+)", "ig")
		if(Needle.test(Pages[i].className))
			Elements[Elements.length] = Pages[i]
	}

	this.Elements = Elements
	this.CurrentPage = 0
	this.ShowPage(this.CurrentPage)

/*	var Elements = []
	for (i in document.all)
	{
		var Needle = new RegExp("(^|\\s+)MenuPage($|\\s+)", "ig")
		if(Needle.test(document.all[i].className))
			Elements[Elements.length] = document.all[i]
	}*/
}

Menu.prototype.ShowPage = function(keyword)
{
	this.CurrentPage = parseInt(keyword)
	for(var i = 0; i < this.Elements.length; i++)
	{
		if(i == this.CurrentPage)
			this.Elements[i].style.display="block"
		else
			this.Elements[i].style.display="none"
	}
}