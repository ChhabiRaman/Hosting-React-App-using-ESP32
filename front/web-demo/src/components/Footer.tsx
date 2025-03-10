interface Props {
  FooterColor: string;
  FooterText: string;
  TextColor: string;
}

const Footer = ({ FooterColor, FooterText, TextColor }: Props) => {
  return (
    <div>
      <footer style={{ backgroundColor: FooterColor, color: TextColor }} className="footer sm:footer-horizontal footer-center p-4 fixed bottom-0">
        <aside>
          <p>
            Copyright © {new Date().getFullYear()} - All right reserved by{" "}
            {FooterText}
          </p>
        </aside>
      </footer>
    </div>
  );
};

export default Footer;
