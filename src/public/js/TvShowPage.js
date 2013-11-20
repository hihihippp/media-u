function TvShowPage() {
    this.tvShow = null;
}

TvShowPage.prototype.bindEvents = function() {
    this.keyDownListener = function(event) {
        ListUtils.handleKeyDown(event);
    }
    window.addEventListener("keydown", this.keyDownListener);
}

TvShowPage.prototype.unbindEvents = function() {
    window.removeEventListener("keydown", this.keyDownListener);
}

TvShowPage.prototype.createNodes = function() {
    var self = this;
    var page = $(".page");
    
    var backButton = $("<input class='backButton' type='button'/>");
    backButton.attr("value", "back");
    backButton.click(function() {
        window.location.hash = "#!/";
    });
    
    this.playButton = PlayButton.create();
    this.playButton.attr("disabled", "disabled");
    
    var completeButton = document.createElement("input");
    completeButton.type = "button";
    completeButton.value = "set completed";
    completeButton.setAttribute("disabled", "disabled");
    
    completeButton.onclick = function() {
        $("li .toggleWatchedButton:not(.watched)").each(function() {
            this.click();
        });
    }
    
    var subtitleTrackField = $("<input type=\"number\"/>");
    var audioTrackField = $("<input type=\"number\"/>");
    $.getJSON("api/page/playerSettings", function(data) {
        subtitleTrackField.get(0).value = data.subtitleTrack;
        audioTrackField.get(0).value = data.audioTrack;
        
        function setFn(key) {
            return function() {
                var number = this.value;
                data[key] = number;
                var json = JSON.stringify(data);
                $.ajax({
                    url:"api/page/playerSettings",
                    type: "PUT",
                    data: json
                }).complete(function() {
                    console.log("set subtitle to ", number);
                });
            }
        }
        
        subtitleTrackField.on("input", setFn("subtitleTrack"));
        audioTrackField.on("input", setFn("audioTrack"));
    });
    
    var episodesEl = $(document.createElement("div"));
    episodesEl.addClass("seasons");
    
    $.getJSON("api/page/showDetails", function(data) {
        console.log(data);
        self.tvShow = data;
        self.playButton.removeAttr("disabled");
        completeButton.removeAttribute("disabled");
        self.createEpisodeList(data.episodes, episodesEl);
    });

    page.append(backButton);
    page.append(this.playButton);
    page.append("<span> sub: </span>");
    page.append(subtitleTrackField);
    page.append("<span> audio: </span>");
    page.append(audioTrackField);
    page.append(episodesEl);
    page.append(completeButton);
    this.bindEvents();
}

TvShowPage.prototype.removeNodes = function() {
    this.unbindEvents();
}

TvShowPage.prototype.createEpisodeList = function(episodes, episodesEl) {
    var self = this;
    var seasonEl = $(document.createElement("ul"));
    seasonEl.addClass("season");
    
    this.episodeList = new EpisodeList(episodes);
    PlayButton.initOnClick(this.playButton, this.episodeList);
     
    $.each(this.episodeList.episodes, function() {
        var ep = this;
        var episodeEl = $(document.createElement("li"));
        var text = $(document.createElement("span"));
        
        var title = 
            ep.episodeNumber + " " +
            ep.showName + " " +
            (ep.episodeName ? ("- " + ep.episodeName + " ") : "") +
            ep.releaseGroup;
        text.text(title);
        
        
        function watchButtonSetDisplay() {
            if (ep.watched) {
                toggleWatchedButton.text("+ ");
                toggleWatchedButton.addClass("watched");
            } else {
                toggleWatchedButton.text("- ");
                toggleWatchedButton.removeClass("watched");
            }
        }
        
        var toggleWatchedButton = $(document.createElement("span"));
        toggleWatchedButton.get(0).className = "textButton toggleWatchedButton";
        watchButtonSetDisplay();
        toggleWatchedButton.click(function(event) {
            ep.watched = !ep.watched;
            watchButtonSetDisplay();
            $.getJSON("api/library/toggleWatched?" + ep.path);
       });
       
        var chapterList = document.createElement("div");
        chapterList.className = "chapterList";
        chapterList.textContent = "-> chapters"
        
        $(chapterList).hover(function() {
            if (ep.metaData) {
                return;
            }
            ep.metaData = {};
            $.getJSON("api/library/movieFileMetaData?" + ep.path, function(data) {
                chapterList.innerHTML = "";
                ep.metaData = data;
                
                $.each(data.chapters, function() {
                    var chapter = this;
                    
                    var chapterLink = document.createElement("span");
                    chapterLink.className = "chapterLink";
                    chapterLink.textContent = chapter.title;
                    chapterList.appendChild(chapterLink);
                    
                    chapterLink.onclick = function(event) {
                        var json = {
                            filename: ep.path
                        };
                        var start = Math.floor(chapter.start);
                        $.getJSON("api/player/play?" + JSON.stringify(json), function(data) {
                            $.getJSON("api/player/jumpTo?" + start, function(data) {
                                window.location.hash = "#!/PlayerPage";
                            });
                        });
                    }
                });
                console.log(ep.metaData);
            });
        });
        
        episodeEl.append(toggleWatchedButton);
        episodeEl.append(text);
        episodeEl.append(chapterList);
        episodeEl.attr("data-fileName", this.path);
        
        episodeEl.click(function(event) {
            if (!event.target.classList.contains("textButton")) {
                self.play($(this).nextAll("li").andSelf().map(function() {
                    return this.getAttribute("data-fileName");
                }).toArray());
            }
        });
        
        episodeEl.attr("tabindex", "1");
        episodeEl.on("focus", function() {
            $(this).mousemove();
        });
        episodeEl.mousemove(function() {
            $("li.focused").removeClass("focused");
            $(this).addClass("focused");
        });
        
        seasonEl.append(episodeEl);
    });
    episodesEl.append(seasonEl);
}

TvShowPage.prototype.play = function(episode) {
    if (G.playerType == "stream") {
        alert("gu")
        G.video = "/video/" + episode; // TODO DON'T SAVE IT HERE
        window.location.hash = "#!/StreamPlayerPage/";
    } else {
        if (episode instanceof Array) {
            var json = {
                tvShow: this.tvShow.name,
                episodes: episode
            }
            
            $.getJSON("api/player/setPlaylist?" + JSON.stringify(json), function(data) {
                if (!data.error) {
                    window.location.hash = "#!/PlayerPage";
                }
            });
        
            /*
            $.ajax({
                url: "api/player/setPlaylist",
                type:"POST",
                data:JSON.stringify(json),
                processData: false
            }).done(function(data) {
                console.log("set playlist got response:", data)
                if (!data.error) {
                    window.location.hash = "#!/PlayerPage";
                }
            });
            */
            
        } else {
            var json = {
                tvShow: this.tvShow.name,
                filename: episode
            }
        
            $.getJSON("api/player/play?" + JSON.stringify(json), function(data) {
                if (!data.error) {
                    window.location.hash = "#!/PlayerPage";
                }
            });
        }
    }
}
