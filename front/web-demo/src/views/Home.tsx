import { useEffect, useState } from "react";
import Card from "../components/Card";
import axios from "axios";

const Home = () => {
  const [info, setInfo] = useState({version: '', cores: 0});
  const image_path = "/logo.png";

  useEffect(() => {
    const getSystemInfo = async () => {
      const systemInfo = await axios.get('/api/v1/system/info')
      .then(res => {
        systemInfo
        setInfo(res.data)
    })
      .catch(err => console.log(err));
    }

    getSystemInfo()
  }, []);

  return (
    <>
      <Card ImagePath={image_path} IdfVer={info.version} EspCores={info.cores} />
    </>
  );
};

export default Home;
