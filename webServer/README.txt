READ ME

1. some ports might not be compatible with my future implementation of post request handling
2. make sure to enter the correct directory without any backslashes and with a forward slash at the end (for webserver directory and website directory)
3. Only put backslashes when compiling the program
4. put website files/folders into the folder named "website"
5. there is already a default error 404 webpage so no need to implement one. if you want to add your own just edit the files as needed
6. the webserver doesn't support php (might be added in future updates)
7. inside the program (webServer.c), you replace the empty strings named defaultDir1 and defaultDir2 with the correct directories (siteDirs.txt directory as defaultDir1) (web server dir as defaultDir2)
8. Make sure to recompile the program after editing the default directories
9. for the database, before and after every row, put a semicolon and seperate columns with spaces(eg: ;row1 column1 column2;)
10. for the database, you cant search for a row using the column command (obviously)
11. also for database, you obviously cant use search with openAppend/opena and you have to include one row variable and at least one column variable
12. if no favicon.ico image is set in the website directory, the program will use a default favicon until a favicon is entered.



siteDirs for multiple websites instructions:

sitedomain is the default site domain that you would use (eg "example.com" you might need to add the port if you arent using port 80).

sitedirectory is the directory to the filder that houses your website. (put '/' at the end of the directory)

directoryroot is the root page that the server will display when no specific page is requested (eg when "example.com/" is requested the default root site would be index.html). this also has to be an html page

start a new site's description with "<" and end it with "/>" (eg "< info />").

you have to format siteDirs.txt like this:

<
	sitedomain website.com
	sitedirectory /directory/to/site
	directoryroot index.html
/>

(sitedomain has to be first but the order of the other paramaters are optional)




RSQL examples for use with db


eg1: OPENR /path/to/folder/with/file/in/it; ROW rowName; COLUMN columnName; COLUMN columnName;

eg2: OPENR /path/to/file; SEARCH true; COLUMN columnName;

seperate each command with a semicolon (and a space only if it is not the last command).

OPENR /path/to/folder/with/file/in/it; opens the file specified by the directory and reads it (OPENR = read/r, OPENW = write/w and OPENA = append/a)

ROW rowName; searches for a row with the name specified by rowName

COLUMN columnName; searches for the column with the name specified by columnName inside the row

if there are more than one column commands, the program will search for all the columns specified in each column command's collumnName

SEARCH true; sets the automatic row search to true. can also be set to false. this takes the input from the collumn command and searches for a row with the column specified by columnName. if there are more than one column commands then the program will search for the row that has the same columns as specified by all the columnName variables

the OPEN command must always be put first then ROW then COLUMNS and SEARCH can be put anywhere after OPEN

there can only be one OPEN call, one SEARCH call and one ROW call



return value for db


5: found requested column
4: found requested row
3: requested column doesnt exist
2: found multiple rows that match
1: found more than one match in the same row
0: success and didnt do anything
-1: couldnt find row
-2: couldnt find end of row/column (db formatted improperly?)
-3: couldnt find column
-4: invalid command name
-5: missing command body
-6: bad file name
-7: unknown search boolean
-8: unknown error






















________IMPORTANT________
the structre of rsql queries is: "<rsql> fid: "id of form to be submitted" cmd: "OPENA /Path/To/Database/File; ROW {name of input 1}; COLUMN {name of input 2};"; </rsql>"

OR

"<rsql> fid: "id of form" cmd: "OPENR /Path/To/Database/File; ROW maybeThisCanBeADefaultRowCounterThatGoesUpEachDatabaseAddition; COLUMN something; COLUMN {name of input}"; </rsql>"

fid = form id,
cmd = command/rsql query
name of input would be: <input type="text" name="this-is-the-input-name">

