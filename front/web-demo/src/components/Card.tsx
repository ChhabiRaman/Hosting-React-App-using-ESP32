interface Props {
  ImagePath: string;
  IdfVer: string;
  EspCores: number;
}

const Card = ({ ImagePath, IdfVer, EspCores }: Props) => {
  return (
    <div className="flex h-screen justify-center items-center">
      <div className="card bg-base-100 w-140 shadow-lg">
        <figure className="px-10 pt-10">
          <img
            src={ImagePath}
            alt="Espressif Logo"
            className="h-45 rounded-xl"
          />
        </figure>
        <div className="card-body items-center text-center">
          <p className="text-gray">IDF version: {IdfVer}</p>
          <p className="text-gray">ESP cores: {EspCores}</p>
        </div>
      </div>
    </div>
  );
};

export default Card;
