char b[242];
char c[60];
int nread 1;
char buf[512];

main(argc,argv) int argc; char *argv[]; {
int l,isw,k,ifile,i,j;

if(--argc <= 0)
	{ifile = 0;
	argc = 0;
	goto newl;
	}

l = 1;
while(argc--)
	{printf("%s:\n \n",argv[l]);
	ifile = open(argv[l++],0);
	if(ifile < 0)
		{printf("cannot open input file\n");
		exit();
		}
	newl:
	isw = j =  0;
	i = -1;
	cont:
		while((b[++i] = get(ifile)) != 0)
			{if((b[i] >= 'a' & b[i] <= 'z') |
			(b[i] >= 'A' & b[i] <= 'Z'))
				{c[j++] = b[i];
				goto cont;
				}
			if(b[i] == '-')
			{c[j++] = b[i];
				if((b[++i] = get(ifile)) != '\n')
				{c[j++] = b[i];
				goto cont;
				}
				if(j == 1)goto newl;
				isw = 1;
				i = -1;
				while(((b[++i] = get(ifile)) == ' ')
				| (b[i] == '\t') | (b[i] == '\n'));
				c[j++] = b[i];
				goto cont;
				}
			if(b[i] == '\n'){if(isw != 1)goto newl;
				i = -1; }
			if(isw == 1)
				{k = 0;
				c[j++] = '\n';
				while(k < j)putchar(c[k++]);
				}
			isw = j = 0;
			}
	}
}
get(ifile) int ifile;{
	char *ibuf;
    static ibuf;

  if(--nread){
    return(*ibuf++);}

  if(nread = read(ifile,buf,512)){
      if(nread < 0)goto err;

    ibuf = buf;
    return(*ibuf++);
    }

  nread = 1;
  return(0);

err:
  nread = 1;
  printf("read error\n");
  return(0);

}
